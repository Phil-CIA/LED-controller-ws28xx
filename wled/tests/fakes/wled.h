#pragma once

#include <ArduinoJson-v6.h>
#include <cassert>
#include <cstdint>
#include <cstring>

#define CONFIG_IDF_TARGET_ESP32 1
#define ESP_ARDUINO_VERSION_MAJOR 2
#define F(text) text
#define DEBUG_PRINTF(...) ((void)0)
#define REGISTER_USERMOD(instance)
constexpr int LOW = 0;
constexpr int OUTPUT = 1;
constexpr int WL_CONNECTED = 3;

enum class PinOwner { None, HW_I2C, UM_Unspecified, BusDigital, HW_SPI };

namespace fake {
static uint32_t now = 1000;
static PinOwner owners[40]{};
static bool ledcUsed[16]{};
static bool ledcOk = true;
static bool allocationOk = true;
static bool updating = false;
static bool sensorOk = true;
static bool resetOk = true;
static unsigned resetCalls = 0;
static unsigned readCalls = 0;
static unsigned pwmCalls = 0;
static unsigned frames = 0;
static int attachedPin = -1;
static unsigned pwm = 0;
static float temperature = 35;
static float humidity = 50;
static uint32_t colors[3]{};
}

static int8_t i2c_sda = 21;
static int8_t i2c_scl = 22;
inline uint32_t millis() { return fake::now; }
inline void digitalWrite(uint8_t, int) {}
inline void pinMode(uint8_t, int) {}
inline void ledcWrite(uint8_t, unsigned duty) {
  ++fake::pwmCalls;
  fake::pwm = duty;
}
inline double ledcSetup(uint8_t channel, double frequency, uint8_t) {
  assert(channel % 2 == 0 && fake::ledcUsed[channel] && fake::ledcUsed[channel + 1]);
  return fake::ledcOk ? frequency : 0;
}
inline void ledcAttachPin(uint8_t pin, uint8_t) { fake::attachedPin = pin; }
inline void ledcDetachPin(uint8_t) { fake::attachedPin = -1; }

namespace PinManager {
inline PinOwner getPinOwner(uint8_t pin) { return fake::owners[pin]; }
inline bool allocatePin(uint8_t pin, bool, PinOwner owner) {
  if (fake::owners[pin] != PinOwner::None) return false;
  fake::owners[pin] = owner;
  return true;
}
inline void deallocatePin(uint8_t pin, PinOwner owner) {
  assert(fake::owners[pin] == owner);
  fake::owners[pin] = PinOwner::None;
}
inline uint8_t allocateLedc(uint8_t count) {
  assert(count == 2);
  for (uint8_t channel = 0; channel < 16; channel += 2) {
    if (!fake::ledcUsed[channel] && !fake::ledcUsed[channel + 1]) {
      fake::ledcUsed[channel] = fake::ledcUsed[channel + 1] = true;
      return channel;
    }
  }
  return 255;
}
inline void deallocateLedc(uint8_t channel, uint8_t count) {
  assert(count == 2 && fake::ledcUsed[channel] && fake::ledcUsed[channel + 1]);
  fake::ledcUsed[channel] = fake::ledcUsed[channel + 1] = false;
}
}

static struct Strip {
  bool isUpdating() const { return fake::updating; }
} strip;

static struct Wifi {
  int connection = WL_CONNECTED;
  int status() const { return connection; }
} WiFi;

class Usermod {
public:
  virtual ~Usermod() {}
  virtual void setup() = 0;
  virtual void loop() = 0;
  virtual void onUpdateBegin(bool) {}
  virtual void addToConfig(JsonObject&) {}
  virtual bool readFromConfig(JsonObject&) { return true; }
  virtual void addToJsonInfo(JsonObject&) {}
};

template<typename T>
bool getJsonValue(JsonVariant value, T& destination) {
  if (value.isNull()) return false;
  destination = value.as<T>();
  return true;
}
