#pragma once

#include "wled.h"

struct RgbColor {
  uint8_t R, G, B;
  explicit RgbColor(uint8_t value) : R(value), G(value), B(value) {}
  RgbColor(uint8_t red, uint8_t green, uint8_t blue) : R(red), G(green), B(blue) {}
};
struct NeoGrbFeature {};
struct NeoEsp32BitBangWs2812xMethod {};

template<typename Feature, typename Method>
class NeoPixelBus {
  uint8_t _buffer[9]{};
public:
  NeoPixelBus(unsigned count, uint8_t pin) {
    assert(count == 3 && pin == 12);
    assert(fake::owners[pin] == PinOwner::UM_Unspecified);
  }
  uint8_t* Pixels() { return fake::allocationOk ? _buffer : nullptr; }
  void Begin() {}
  bool CanShow() { return true; }
  void ClearTo(RgbColor color) {
    for (unsigned i = 0; i < 3; ++i) SetPixelColor(i, color);
  }
  void SetPixelColor(unsigned index, RgbColor color) {
    assert(index < 3);
    fake::colors[index] = (uint32_t(color.R) << 16) | (uint32_t(color.G) << 8) | color.B;
  }
  void Show() { ++fake::frames; }
};
