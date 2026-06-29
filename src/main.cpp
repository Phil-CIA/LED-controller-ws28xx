#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_AHTX0.h>
#include <Wire.h>

#if defined(LED_DATA_PIN)
constexpr uint8_t kLedDataPin = LED_DATA_PIN;
#else
constexpr uint8_t kLedDataPin = 18;
#endif

#if defined(LED_COUNT)
constexpr uint16_t kLedCount = LED_COUNT;
#else
constexpr uint16_t kLedCount = 300;
#endif

#if defined(LED_BRIGHTNESS)
const uint8_t kDefaultBrightness = LED_BRIGHTNESS;
#else
const uint8_t kDefaultBrightness = 32;
#endif

#if defined(ONBOARD_LED_PIN)
const uint8_t kOnboardLedPin = ONBOARD_LED_PIN;
#else
const uint8_t kOnboardLedPin = 19;
#endif

#if defined(ONBOARD_LED_COUNT)
const uint16_t kOnboardLedCount = ONBOARD_LED_COUNT;
#else
const uint16_t kOnboardLedCount = 3;
#endif

#if defined(AHT20_SDA_PIN)
const uint8_t kAht20SdaPin = AHT20_SDA_PIN;
#else
const uint8_t kAht20SdaPin = 21;
#endif

#if defined(AHT20_SCL_PIN)
const uint8_t kAht20SclPin = AHT20_SCL_PIN;
#else
const uint8_t kAht20SclPin = 22;
#endif

#if defined(SWITCH1_PIN)
const uint8_t kSwitchPins[4] = {SWITCH1_PIN, SWITCH2_PIN, SWITCH3_PIN, SWITCH4_PIN};
#else
const uint8_t kSwitchPins[4] = {34, 35, 36, 39};
#endif

#if defined(SWITCH_ACTIVE_LEVEL)
const uint8_t kSwitchActiveLevel = SWITCH_ACTIVE_LEVEL;
#else
const uint8_t kSwitchActiveLevel = LOW;
#endif

#if defined(FIRMWARE_PROFILE_NAME)
const char* kFirmwareProfileName = FIRMWARE_PROFILE_NAME;
#else
const char* kFirmwareProfileName = "unknown";
#endif

const uint16_t kWhiteMilliAmpsPerLed = 60;
const uint32_t kEstimatedFullWhiteMilliAmps = static_cast<uint32_t>(kLedCount) * kWhiteMilliAmpsPerLed;
const uint16_t kSteadyCurrentLimitMa = 12000;
const uint16_t kHardMaxCurrentLimitMa = 15000;

Adafruit_NeoPixel strip(kLedCount, kLedDataPin, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel onboardStrip(kOnboardLedCount, kOnboardLedPin, NEO_GRB + NEO_KHZ800);
Adafruit_AHTX0 aht20;

enum class LedMode : uint8_t {
  Rainbow,
  Demo,
  Solid,
  Off
};

uint8_t userBrightness = kDefaultBrightness;
uint8_t brightnessCap = 255;
uint16_t hueOffset = 0;
uint32_t lastFrameMs = 0;
LedMode mode = LedMode::Rainbow;
uint32_t solidColor = 0;
uint32_t demoStartMs = 0;
uint16_t demoStep = 0;
bool aht20Ready = false;
uint8_t switchStableState[4] = {0, 0, 0, 0};
uint8_t switchLastSample[4] = {0, 0, 0, 0};
uint32_t switchLastDebounceMs[4] = {0, 0, 0, 0};
constexpr uint32_t kSwitchDebounceMs = 30;

static uint8_t effectiveBrightness() {
  return min(userBrightness, brightnessCap);
}

static uint8_t currentLimitToBrightnessCap(uint16_t currentLimitMa) {
  if (currentLimitMa >= kEstimatedFullWhiteMilliAmps) {
    return 255;
  }

  const uint32_t cappedBrightness = (static_cast<uint32_t>(currentLimitMa) * 255U + kEstimatedFullWhiteMilliAmps - 1U) / kEstimatedFullWhiteMilliAmps;
  return static_cast<uint8_t>(constrain(cappedBrightness, 1UL, 255UL));
}

static void applyBrightness() {
  strip.setBrightness(effectiveBrightness());
  onboardStrip.setBrightness(effectiveBrightness());
}

static void initBoardInputs() {
  const uint32_t now = millis();
  for (uint8_t i = 0; i < 4; ++i) {
    pinMode(kSwitchPins[i], INPUT);
    const uint8_t sample = static_cast<uint8_t>(digitalRead(kSwitchPins[i]));
    switchStableState[i] = sample;
    switchLastSample[i] = sample;
    switchLastDebounceMs[i] = now;
  }
}

static void pollSwitchPresses(uint32_t nowMs) {
  for (uint8_t i = 0; i < 4; ++i) {
    const uint8_t sample = static_cast<uint8_t>(digitalRead(kSwitchPins[i]));

    if (sample != switchLastSample[i]) {
      switchLastSample[i] = sample;
      switchLastDebounceMs[i] = nowMs;
    }

    if ((nowMs - switchLastDebounceMs[i] >= kSwitchDebounceMs) && (sample != switchStableState[i])) {
      switchStableState[i] = sample;
      if (sample == kSwitchActiveLevel) {
        Serial.printf("Switch S%u pressed\n", i + 1);
      }
    }
  }
}

static void initAht20() {
  Wire.begin(kAht20SdaPin, kAht20SclPin);
  aht20Ready = aht20.begin(&Wire);
}

static void printSwitchStates() {
  Serial.printf("Switches (raw): S1=%d S2=%d S3=%d S4=%d\n",
                digitalRead(kSwitchPins[0]),
                digitalRead(kSwitchPins[1]),
                digitalRead(kSwitchPins[2]),
                digitalRead(kSwitchPins[3]));
}

static void printAht20Reading() {
  if (!aht20Ready) {
    initAht20();
  }

  if (!aht20Ready) {
    Serial.printf("AHT20 not detected on I2C SDA=%u SCL=%u\n", kAht20SdaPin, kAht20SclPin);
    return;
  }

  sensors_event_t humidity;
  sensors_event_t temp;
  aht20.getEvent(&humidity, &temp);
  Serial.printf("AHT20: temp=%.2f C humidity=%.2f %%RH\n", temp.temperature, humidity.relative_humidity);
}

static void scanI2cBus() {
  Wire.begin(kAht20SdaPin, kAht20SclPin);
  Serial.printf("Scanning I2C bus on SDA=%u SCL=%u...\n", kAht20SdaPin, kAht20SclPin);

  uint8_t foundCount = 0;
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    const uint8_t error = Wire.endTransmission();
    if (error == 0) {
      Serial.printf("I2C device found at 0x%02X\n", address);
      ++foundCount;
    }
  }

  if (foundCount == 0) {
    Serial.println("No I2C devices found");
  } else {
    Serial.printf("I2C scan complete: %u device(s) found\n", foundCount);
  }
}

static void pulseProbePin() {
  pinMode(kLedDataPin, OUTPUT);
  digitalWrite(kLedDataPin, LOW);
  delay(100);
  for (uint8_t i = 0; i < 8; ++i) {
    digitalWrite(kLedDataPin, HIGH);
    delay(80);
    digitalWrite(kLedDataPin, LOW);
    delay(80);
  }
}

static uint32_t wheel(uint8_t wheelPos) {
  wheelPos = 255 - wheelPos;
  if (wheelPos < 85) {
    return strip.Color(255 - wheelPos * 3, 0, wheelPos * 3);
  }
  if (wheelPos < 170) {
    wheelPos -= 85;
    return strip.Color(0, wheelPos * 3, 255 - wheelPos * 3);
  }
  wheelPos -= 170;
  return strip.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}

static void clearStrip() {
  strip.clear();
  strip.show();
  onboardStrip.clear();
  onboardStrip.show();
}

static void showSolid(uint32_t color) {
  for (uint16_t i = 0; i < kLedCount; ++i) {
    strip.setPixelColor(i, color);
  }
  strip.show();
  for (uint16_t i = 0; i < kOnboardLedCount; ++i) {
    onboardStrip.setPixelColor(i, color);
  }
  onboardStrip.show();
}

static void renderRainbowStep() {
  for (uint16_t i = 0; i < kLedCount; ++i) {
    const uint8_t colorIndex = static_cast<uint8_t>((i * 256U / kLedCount + hueOffset) & 0xFFU);
    strip.setPixelColor(i, wheel(colorIndex));
  }
  strip.show();
  for (uint16_t i = 0; i < kOnboardLedCount; ++i) {
    const uint8_t colorIndex = static_cast<uint8_t>((i * 256U / kOnboardLedCount + hueOffset) & 0xFFU);
    onboardStrip.setPixelColor(i, wheel(colorIndex));
  }
  onboardStrip.show();
  hueOffset++;
}

static void renderDemoStep(uint32_t nowMs) {
  constexpr uint32_t kPhaseDurationMs = 7000;
  const uint32_t elapsedMs = nowMs - demoStartMs;
  const uint8_t phase = static_cast<uint8_t>((elapsedMs / kPhaseDurationMs) % 4U);
  const uint16_t localStep = static_cast<uint16_t>(elapsedMs / 18U);

  if (phase == 0) {
    for (uint16_t i = 0; i < kLedCount; ++i) {
      const uint8_t colorIndex = static_cast<uint8_t>((i * 512U / kLedCount + localStep * 2U) & 0xFFU);
      strip.setPixelColor(i, wheel(colorIndex));
    }
    strip.show();
    for (uint16_t i = 0; i < kOnboardLedCount; ++i) {
      const uint8_t colorIndex = static_cast<uint8_t>((i * 512U / kOnboardLedCount + localStep * 2U) & 0xFFU);
      onboardStrip.setPixelColor(i, wheel(colorIndex));
    }
    onboardStrip.show();
    return;
  }

  if (phase == 1) {
    const uint8_t chaseOffset = static_cast<uint8_t>(localStep % 9U);
    const uint32_t palette[3] = {
      strip.Color(255, 10, 0),
      strip.Color(0, 180, 255),
      strip.Color(40, 255, 40)
    };

    for (uint16_t i = 0; i < kLedCount; ++i) {
      if (((i + chaseOffset) % 9U) < 3U) {
        strip.setPixelColor(i, palette[(i / 3U + localStep) % 3U]);
      } else {
        strip.setPixelColor(i, 0);
      }
    }
    strip.show();
    for (uint16_t i = 0; i < kOnboardLedCount; ++i) {
      if (((i + chaseOffset) % 9U) < 3U) {
        onboardStrip.setPixelColor(i, palette[(i / 3U + localStep) % 3U]);
      } else {
        onboardStrip.setPixelColor(i, 0);
      }
    }
    onboardStrip.show();
    return;
  }

  if (phase == 2) {
    const uint16_t cometHead = static_cast<uint16_t>((localStep * 3U) % kLedCount);
    for (uint16_t i = 0; i < kLedCount; ++i) {
      const uint16_t distance = static_cast<uint16_t>((kLedCount + cometHead - i) % kLedCount);
      if (distance < 24U) {
        const uint8_t fade = static_cast<uint8_t>(255U - distance * 10U);
        strip.setPixelColor(i, strip.Color(fade, fade / 6U, fade));
      } else {
        strip.setPixelColor(i, 0);
      }
    }
    strip.show();
    const uint16_t onboardCometHead = static_cast<uint16_t>((localStep * 3U) % kOnboardLedCount);
    for (uint16_t i = 0; i < kOnboardLedCount; ++i) {
      const uint16_t distance = static_cast<uint16_t>((kOnboardLedCount + onboardCometHead - i) % kOnboardLedCount);
      if (distance < 24U) {
        const uint8_t fade = static_cast<uint8_t>(255U - distance * 10U);
        onboardStrip.setPixelColor(i, onboardStrip.Color(fade, fade / 6U, fade));
      } else {
        onboardStrip.setPixelColor(i, 0);
      }
    }
    onboardStrip.show();
    return;
  }

  const bool flashOn = ((localStep / 3U) % 2U) == 0U;
  if (flashOn) {
    const uint32_t pulseColor = wheel(static_cast<uint8_t>((localStep * 5U) & 0xFFU));
    for (uint16_t i = 0; i < kLedCount; ++i) {
      strip.setPixelColor(i, pulseColor);
    }
    for (uint16_t i = 0; i < kOnboardLedCount; ++i) {
      onboardStrip.setPixelColor(i, pulseColor);
    }
  } else {
    for (uint16_t i = 0; i < kLedCount; ++i) {
      strip.setPixelColor(i, 0);
    }
    for (uint16_t i = 0; i < kOnboardLedCount; ++i) {
      onboardStrip.setPixelColor(i, 0);
    }
  }
  strip.show();
  onboardStrip.show();
}

static void runStartupDiagnostic() {
  const uint32_t colors[] = {
    strip.Color(255, 0, 0),
    strip.Color(0, 255, 0),
    strip.Color(0, 0, 255),
    strip.Color(255, 255, 255)
  };

  for (uint8_t c = 0; c < 4; ++c) {
    showSolid(colors[c]);
    delay(350);
    clearStrip();
    delay(120);
  }
  Serial.println("Startup diagnostic: on-board 3 LEDs + external strip tested RGBW");
}

static void handleSerial() {
  if (!Serial.available()) {
    return;
  }

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "off") {
    mode = LedMode::Off;
    clearStrip();
    Serial.println("LEDs off");
    return;
  }

  if (cmd == "on") {
    mode = LedMode::Rainbow;
    applyBrightness();
    Serial.println("LED animation on (rainbow)");
    return;
  }

  if (cmd == "demo") {
    mode = LedMode::Demo;
    demoStartMs = millis();
    demoStep = 0;
    applyBrightness();
    Serial.println("Mode: demo");
    return;
  }

  if (cmd == "rainbow") {
    mode = LedMode::Rainbow;
    Serial.println("Mode: rainbow");
    return;
  }

  if (cmd == "white") {
    mode = LedMode::Solid;
    solidColor = strip.Color(255, 255, 255);
    showSolid(solidColor);
    Serial.println("Mode: solid white");
    return;
  }

  if (cmd == "red") {
    mode = LedMode::Solid;
    solidColor = strip.Color(255, 0, 0);
    showSolid(solidColor);
    Serial.println("Mode: solid red");
    return;
  }

  if (cmd == "green") {
    mode = LedMode::Solid;
    solidColor = strip.Color(0, 255, 0);
    showSolid(solidColor);
    Serial.println("Mode: solid green");
    return;
  }

  if (cmd == "blue") {
    mode = LedMode::Solid;
    solidColor = strip.Color(0, 0, 255);
    showSolid(solidColor);
    Serial.println("Mode: solid blue");
    return;
  }

  if (cmd == "probe") {
    Serial.println("Running raw GPIO probe pulse on data pin");
    pulseProbePin();
    strip.begin();
    applyBrightness();
    Serial.println("Probe done");
    return;
  }

  if (cmd == "aht20" || cmd == "sensor") {
    printAht20Reading();
    return;
  }

  if (cmd == "i2cscan" || cmd == "scan") {
    scanI2cBus();
    return;
  }

  if (cmd == "switches" || cmd == "switch" || cmd == "inputs") {
    printSwitchStates();
    return;
  }

  if (cmd == "boardcheck" || cmd == "diag") {
    printAht20Reading();
    printSwitchStates();
    return;
  }

  if (cmd == "test12" || cmd == "12a") {
    mode = LedMode::Solid;
    userBrightness = 255;
    brightnessCap = currentLimitToBrightnessCap(12000);
    solidColor = strip.Color(255, 255, 255);
    applyBrightness();
    showSolid(solidColor);
    Serial.printf("12A test active: brightness cap %u, effective brightness %u\n", brightnessCap, effectiveBrightness());
    return;
  }

  if (cmd == "nolimit") {
    brightnessCap = currentLimitToBrightnessCap(kHardMaxCurrentLimitMa);
    applyBrightness();
    if (mode == LedMode::Solid) {
      showSolid(solidColor);
    }
    Serial.printf("Current limit set to hard max %u mA\n", kHardMaxCurrentLimitMa);
    return;
  }

  if (cmd.startsWith("limit")) {
    int value = cmd.substring(5).toInt();
    value = constrain(value, 0, static_cast<int>(kHardMaxCurrentLimitMa));
    brightnessCap = currentLimitToBrightnessCap(static_cast<uint16_t>(value));
    applyBrightness();
    if (mode == LedMode::Solid) {
      showSolid(solidColor);
    }
    Serial.printf("Current limit set to %d mA; brightness cap %u; effective brightness %u\n", value, brightnessCap, effectiveBrightness());
    return;
  }

  if (cmd.startsWith("b")) {
    int value = cmd.substring(1).toInt();
    value = constrain(value, 0, 255);
    userBrightness = static_cast<uint8_t>(value);
    applyBrightness();
    if (mode == LedMode::Solid) {
      showSolid(solidColor);
    }
    Serial.printf("Brightness set to %u (effective %u)\n", userBrightness, effectiveBrightness());
    return;
  }

  Serial.println("Commands: on|demo|rainbow|off|white|red|green|blue|probe|aht20|sensor|i2cscan|scan|switches|boardcheck|test12|12a|limit<ma>|nolimit|b<0-255>");
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pulseProbePin();
  initBoardInputs();
  initAht20();
  strip.begin();
  onboardStrip.begin();
  brightnessCap = currentLimitToBrightnessCap(kSteadyCurrentLimitMa);
  applyBrightness();
  clearStrip();
  runStartupDiagnostic();

  Serial.println();
  Serial.println("My LED Controller ready");
  Serial.printf("Firmware profile: %s\n", kFirmwareProfileName);
  Serial.printf("External strip: GPIO%u, LED count: %u\n", kLedDataPin, kLedCount);
  Serial.printf("On-board LEDs: GPIO%u, LED count: %u\n", kOnboardLedPin, kOnboardLedCount);
  Serial.printf("AHT20 I2C: SDA=%u SCL=%u (%s)\n", kAht20SdaPin, kAht20SclPin, aht20Ready ? "ready" : "not detected");
  Serial.printf("Switch inputs: GPIO%u GPIO%u GPIO%u GPIO%u\n", kSwitchPins[0], kSwitchPins[1], kSwitchPins[2], kSwitchPins[3]);
  Serial.printf("Switch logging: active level %u, debounce %lu ms\n", kSwitchActiveLevel, static_cast<unsigned long>(kSwitchDebounceMs));
  Serial.printf("Brightness: %u, steady limit: %u mA, max limit: %u mA\n", userBrightness, kSteadyCurrentLimitMa, kHardMaxCurrentLimitMa);
  Serial.println("Commands: on|demo|rainbow|off|white|red|green|blue|probe|aht20|sensor|i2cscan|scan|switches|boardcheck|test12|12a|limit<ma>|nolimit|b<0-255>");
}

void loop() {
  const uint32_t now = millis();
  pollSwitchPresses(now);
  handleSerial();

  if (mode == LedMode::Rainbow && now - lastFrameMs >= 20) {
    lastFrameMs = now;
    renderRainbowStep();
  }

  if (mode == LedMode::Demo && now - lastFrameMs >= 18) {
    lastFrameMs = now;
    demoStep++;
    renderDemoStep(now);
  }

  if (mode == LedMode::Solid) {
    delay(10);
  }
}
