#pragma once

#include <cmath>
#include <cstdint>

namespace rev_b {

constexpr uint32_t sampleIntervalMs = 5000;
constexpr uint32_t staleAfterMs = 30000;
constexpr uint32_t startKickMs = 1000;

enum class SensorHealth { Waiting, Healthy, Failed, Stale };

class Sample {
public:
  void reset(uint32_t now = 0) {
    *this = Sample();
    _waitingSince = now;
  }

  bool record(float temperature, float humidity, uint32_t now) {
    if (!std::isfinite(temperature) || temperature < -40.0f || temperature > 85.0f ||
        !std::isfinite(humidity) || humidity < 0.0f || humidity > 100.0f) {
      fail();
      return false;
    }
    _attempted = _valid = _hasReading = true;
    _temperature = temperature;
    _humidity = humidity;
    _lastSuccess = now;
    return true;
  }

  void fail() {
    _attempted = true;
    _valid = false;
  }

  SensorHealth health(uint32_t now) const {
    if (!_attempted) return uint32_t(now - _waitingSince) >= staleAfterMs ?
                            SensorHealth::Stale : SensorHealth::Waiting;
    if (!_valid) return SensorHealth::Failed;
    return age(now) >= staleAfterMs ? SensorHealth::Stale : SensorHealth::Healthy;
  }

  bool hasReading() const { return _hasReading; }
  uint32_t age(uint32_t now) const { return now - _lastSuccess; }
  float temperature() const { return _temperature; }
  float humidity() const { return _humidity; }

private:
  bool _attempted = false;
  bool _valid = false;
  bool _hasReading = false;
  uint32_t _waitingSince = 0;
  uint32_t _lastSuccess = 0;
  float _temperature = 0;
  float _humidity = 0;
};

class FanPolicy {
public:
  void reset() { *this = FanPolicy(); }

  uint8_t update(float temperature, bool healthy, uint32_t now) {
    if (!healthy || !std::isfinite(temperature)) {
      _step = 0;
      _kicking = false;
      _lastDuty = 100;
      return _lastDuty;
    }

    const float boundaries[] = {30.0f, 35.0f, 40.0f, 45.0f};
    const uint8_t duties[] = {0, 40, 60, 80, 100};
    while (_step < 4 && temperature >= boundaries[_step]) ++_step;
    while (_step > 0 && temperature <= boundaries[_step - 1] - 2.0f) --_step;

    const uint8_t requested = duties[_step];
    if (requested == 0) {
      _kicking = false;
    } else if (_lastDuty == 0) {
      _kicking = true;
      _kickStarted = now;
    }
    if (_kicking && uint32_t(now - _kickStarted) >= startKickMs) _kicking = false;
    _lastDuty = _kicking ? 100 : requested;
    return _lastDuty;
  }

private:
  uint8_t _step = 0;
  uint8_t _lastDuty = 0;
  bool _kicking = false;
  uint32_t _kickStarted = 0;
};

struct IndicatorFrame {
  uint32_t power;
  uint32_t wifi;
  uint32_t status;
};

inline IndicatorFrame indicators(bool wifiConnected, SensorHealth health,
                                 uint32_t now, uint8_t brightness) {
  const uint32_t red = uint32_t(brightness) << 16;
  const uint32_t green = uint32_t(brightness) << 8;
  const uint32_t blue = brightness;
  const uint32_t amber = red | (uint32_t(brightness / 2) << 8);
  const bool blinkOn = (now / 500) % 2 == 0;
  const uint32_t status = health == SensorHealth::Healthy ? 0 :
                          health == SensorHealth::Waiting ? amber : red;
  // Physical chain order is D1 power, D2 Wi-Fi, D3 status, not the silkscreen.
  return {green, wifiConnected ? blue : (blinkOn ? amber : 0), status};
}

} // namespace rev_b
