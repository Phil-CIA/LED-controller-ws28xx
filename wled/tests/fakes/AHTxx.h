#pragma once

#include "wled.h"

constexpr uint8_t AHTXX_ADDRESS_X38 = 0x38;
constexpr uint8_t AHT2x_SENSOR = 1;
constexpr uint8_t AHTXX_NO_ERROR = 0;
constexpr bool AHTXX_USE_READ_DATA = false;

class AHTxx {
public:
  AHTxx(uint8_t address, uint8_t type) {
    assert(address == 0x38 && type == AHT2x_SENSOR);
  }
  bool softReset() {
    ++fake::resetCalls;
    return fake::resetOk;
  }
  float readTemperature() {
    ++fake::readCalls;
    return fake::temperature;
  }
  float readHumidity(bool forceRead) {
    assert(!forceRead);
    return fake::humidity;
  }
  uint8_t getStatus() { return fake::sensorOk ? AHTXX_NO_ERROR : 4; }
};
