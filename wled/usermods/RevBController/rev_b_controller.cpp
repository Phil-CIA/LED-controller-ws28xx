#include <NeoPixelBus.h>
#include "wled.h"
#include <AHTxx.h>
#include <memory>
#include <new>
#include "rev_b_policy.h"

#if !defined(CONFIG_IDF_TARGET_ESP32) || ESP_ARDUINO_VERSION_MAJOR != 2
#error RevBController requires classic ESP32 and Arduino-ESP32 2.x (WLED v16.0.1).
#endif

class RevBController : public Usermod {
  using StatusPixels = NeoPixelBus<NeoGrbFeature, NeoEsp32BitBangWs2812xMethod>;
  static constexpr uint8_t indicatorPin = 12;
  static constexpr uint8_t fanPin = 23;
  static constexpr uint8_t noChannel = 255;

  bool _sensorEnabled = false;
  bool _indicatorsEnabled = false;
  bool _fanEnabled = false;
  int _brightness = 16;
  int _pwmHz = 0;
  bool _started = false;
  bool _reconfigure = false;
  bool _updating = false;
  bool _sensorReady = false;
  uint32_t _lastAttempt = 0;
  uint8_t _pwmChannel = noChannel;
  uint8_t _fanDuty = 0;
  const char* _sensorMessage = "Disabled";
  const char* _indicatorMessage = "Disabled";
  const char* _fanMessage = "Disabled";
  AHTxx _sensor{AHTXX_ADDRESS_X38, AHT2x_SENSOR};
  rev_b::Sample _sample;
  rev_b::FanPolicy _fanPolicy;
  std::unique_ptr<StatusPixels> _pixels;
  rev_b::IndicatorFrame _lastFrame{0, 0, 0};
  bool _frameSent = false;

  void report(const char*& destination, const char* message) {
    if (strcmp(destination, message) != 0) {
      DEBUG_PRINTF("[RevBController] %s\n", message);
      destination = message;
    }
  }

  bool i2cReady() const {
    return i2c_sda == 21 && i2c_scl == 22 &&
           PinManager::getPinOwner(21) == PinOwner::HW_I2C &&
           PinManager::getPinOwner(22) == PinOwner::HW_I2C;
  }

  void writeFan(uint8_t duty) {
    if (_pwmChannel == noChannel) return;
    ledcWrite(_pwmChannel, (unsigned(duty) * 255 + 50) / 100);
    _fanDuty = duty;
  }

  void stopFan() {
    if (_pwmChannel == noChannel) return;
    writeFan(0);
    ledcDetachPin(fanPin);
    digitalWrite(fanPin, LOW);
    pinMode(fanPin, OUTPUT);
    PinManager::deallocateLedc(_pwmChannel, 2);
    PinManager::deallocatePin(fanPin, PinOwner::UM_Unspecified);
    _pwmChannel = noChannel;
  }

  void startFan() {
    if (!_fanEnabled) {
      report(_fanMessage, "Disabled");
      return;
    }
    if (_pwmHz < 20 || _pwmHz > 25000) {
      report(_fanMessage, "Set a verified PWM frequency (20-25000 Hz)");
      return;
    }
    if (!PinManager::allocatePin(fanPin, true, PinOwner::UM_Unspecified)) {
      report(_fanMessage, "GPIO23 is already in use; fan not started");
      return;
    }
    // Reserve both channels of a timer so another output cannot change its frequency.
    _pwmChannel = PinManager::allocateLedc(2);
    if (_pwmChannel == noChannel) {
      PinManager::deallocatePin(fanPin, PinOwner::UM_Unspecified);
      report(_fanMessage, "No free LEDC timer; fan not started");
      return;
    }
    if (ledcSetup(_pwmChannel, _pwmHz, 8) == 0) {
      PinManager::deallocateLedc(_pwmChannel, 2);
      PinManager::deallocatePin(fanPin, PinOwner::UM_Unspecified);
      _pwmChannel = noChannel;
      report(_fanMessage, "PWM setup failed; fan not started");
      return;
    }
    ledcWrite(_pwmChannel, 255);
    ledcAttachPin(fanPin, _pwmChannel);
    _fanDuty = 100;
    _fanPolicy.reset();
    report(_fanMessage, "Active");
  }

  void stopIndicators() {
    if (!_pixels) return;
    _pixels->ClearTo(RgbColor(0));
    _pixels->Show();
    _pixels.reset();
    PinManager::deallocatePin(indicatorPin, PinOwner::UM_Unspecified);
  }

  void startIndicators() {
    if (!_indicatorsEnabled) {
      report(_indicatorMessage, "Disabled");
      return;
    }
    if (_brightness < 0 || _brightness > 64) {
      report(_indicatorMessage, "Indicator brightness must be 0-64");
      return;
    }
    if (!PinManager::allocatePin(indicatorPin, true, PinOwner::UM_Unspecified)) {
      report(_indicatorMessage, "GPIO12 is already in use; remove its WLED output first");
      return;
    }
    // Three software-driven pixels do not share WLED's resettable RMT/I2S allocator.
    _pixels.reset(new (std::nothrow) StatusPixels(3, indicatorPin));
    if (!_pixels || !_pixels->Pixels()) {
      _pixels.reset();
      PinManager::deallocatePin(indicatorPin, PinOwner::UM_Unspecified);
      report(_indicatorMessage, "Cannot allocate indicator pixels");
      return;
    }
    _pixels->Begin();
    _frameSent = false;
    report(_indicatorMessage, "Active: D1 power, D2 Wi-Fi, D3 status");
  }

  void applyConfig() {
    stopFan();
    stopIndicators();
    _sample.reset(millis());
    _sensorReady = false;
    _lastAttempt = millis() - rev_b::sampleIntervalMs;
    report(_sensorMessage, _sensorEnabled ? "Waiting for first sample" : "Disabled");
    if (!_sensorEnabled) _sample.fail();
    startFan();
    startIndicators();
    _reconfigure = false;
  }

  void readSensor(uint32_t now) {
    if (!_sensorEnabled) return;
    if (!i2cReady()) {
      _sample.fail();
      _sensorReady = false;
      report(_sensorMessage, "Set WLED I2C SDA/SCL to GPIO21/22 and reboot");
      return;
    }
    if (uint32_t(now - _lastAttempt) < rev_b::sampleIntervalMs || strip.isUpdating()) return;
    _lastAttempt = now;
    // WLED owns and initializes Wire; only reset/calibrate the AHT20, not the I2C bus.
    if (!_sensorReady && !_sensor.softReset()) {
      _sample.fail();
      report(_sensorMessage, "AHT20 missing or initialization failed");
      return;
    }
    _sensorReady = true;
    const float temperature = _sensor.readTemperature();
    const float humidity = _sensor.readHumidity(AHTXX_USE_READ_DATA);
    if (_sensor.getStatus() != AHTXX_NO_ERROR) {
      _sample.fail();
      _sensorReady = false;
      report(_sensorMessage, "AHT20 read failed (I2C, busy or CRC error)");
      return;
    }
    if (!_sample.record(temperature, humidity, millis())) {
      _sensorReady = false;
      report(_sensorMessage, "AHT20 returned invalid temperature/humidity");
      return;
    }
    report(_sensorMessage, "Healthy");
  }

  void showIndicators(uint32_t now) {
    if (!_pixels || !_pixels->CanShow()) return;
    auto health = _sample.health(now);
    if (_fanEnabled && _pwmChannel == noChannel) health = rev_b::SensorHealth::Failed;
    const auto frame = rev_b::indicators(WiFi.status() == WL_CONNECTED, health, now, _brightness);
    if (_frameSent && frame.power == _lastFrame.power && frame.wifi == _lastFrame.wifi &&
        frame.status == _lastFrame.status) return;
    const uint32_t colors[] = {frame.power, frame.wifi, frame.status};
    for (unsigned i = 0; i < 3; ++i) {
      const uint32_t c = colors[i];
      _pixels->SetPixelColor(i, RgbColor((c >> 16) & 255, (c >> 8) & 255, c & 255));
    }
    _pixels->Show();
    _lastFrame = frame;
    _frameSent = true;
  }

public:
  ~RevBController() override {
    stopFan();
    stopIndicators();
  }

  void setup() override {
    _started = true;
    applyConfig();
  }

  void loop() override {
    if (_reconfigure) applyConfig();
    if (!_updating) readSensor(millis());
    const uint32_t now = millis();
    if (_pwmChannel != noChannel) {
      writeFan(_updating ? 100 : _fanPolicy.update(
        _sample.temperature(), _sample.health(now) == rev_b::SensorHealth::Healthy, now));
    }
    showIndicators(now);
  }

  void onUpdateBegin(bool updating) override {
    _updating = updating;
    if (updating) writeFan(100);
  }

  void addToConfig(JsonObject& root) override {
    JsonObject config = root.createNestedObject(F("RevBController"));
    config[F("sensor-enabled")] = _sensorEnabled;
    config[F("indicators-enabled")] = _indicatorsEnabled;
    config[F("fan-enabled")] = _fanEnabled;
    config[F("indicator-brightness")] = _brightness;
    config[F("fan-pwm-Hz")] = _pwmHz;
  }

  bool readFromConfig(JsonObject& root) override {
    JsonObject config = root[F("RevBController")];
    if (config.isNull()) return false;
    bool complete = getJsonValue(config[F("sensor-enabled")], _sensorEnabled);
    complete &= getJsonValue(config[F("indicators-enabled")], _indicatorsEnabled);
    complete &= getJsonValue(config[F("fan-enabled")], _fanEnabled);
    complete &= getJsonValue(config[F("indicator-brightness")], _brightness);
    complete &= getJsonValue(config[F("fan-pwm-Hz")], _pwmHz);
    if (_started) _reconfigure = true;
    return complete;
  }

  void addToJsonInfo(JsonObject& root) override {
    JsonObject info = root[F("u")];
    if (info.isNull()) info = root.createNestedObject(F("u"));
    const uint32_t now = millis();
    const auto health = _sample.health(now);
    JsonArray sensor = info.createNestedArray(F("Rev B sensor"));
    sensor.add(health == rev_b::SensorHealth::Stale ? "Stale (30 seconds without a reading)" : _sensorMessage);
    if (health == rev_b::SensorHealth::Healthy) {
      JsonArray temperature = info.createNestedArray(F("Temperature"));
      temperature.add(_sample.temperature());
      temperature.add(F(" C"));
      JsonArray humidity = info.createNestedArray(F("Humidity"));
      humidity.add(_sample.humidity());
      humidity.add(F("%"));
    }
    if (_sample.hasReading()) {
      JsonArray age = info.createNestedArray(F("Rev B sample age"));
      age.add(_sample.age(now) / 1000);
      age.add(F("s"));
    }
    JsonArray fan = info.createNestedArray(F("Rev B fan"));
    if (_pwmChannel == noChannel) {
      fan.add(_fanMessage);
    } else {
      fan.add(_fanDuty);
      fan.add(F("%"));
      fan.add(_updating || health != rev_b::SensorHealth::Healthy ? "Fail-safe" : "Automatic");
    }
    JsonArray indicators = info.createNestedArray(F("Rev B indicators"));
    indicators.add(_indicatorMessage);
  }
};

static RevBController revBController;
REGISTER_USERMOD(revBController);
