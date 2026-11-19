#include "../usermods/RevBController/rev_b_policy.h"
#include <cassert>
#include <cstdio>
#include <limits>

using rev_b::FanPolicy;
using rev_b::Sample;
using rev_b::SensorHealth;

static void curveAndHysteresis() {
  FanPolicy fan;
  assert(fan.update(29.99f, true, 0) == 0);
  assert(fan.update(30, true, 1) == 100);
  assert(fan.update(30, true, 1000) == 100);
  assert(fan.update(30, true, 1001) == 40);
  assert(fan.update(34.99f, true, 1002) == 40);
  assert(fan.update(35, true, 1003) == 60);
  assert(fan.update(39.99f, true, 1004) == 60);
  assert(fan.update(40, true, 1005) == 80);
  assert(fan.update(44.99f, true, 1006) == 80);
  assert(fan.update(45, true, 1007) == 100);
  assert(fan.update(43.01f, true, 1008) == 100);
  assert(fan.update(43, true, 1009) == 80);
  assert(fan.update(38.01f, true, 1010) == 80);
  assert(fan.update(38, true, 1011) == 60);
  assert(fan.update(33.01f, true, 1012) == 60);
  assert(fan.update(33, true, 1013) == 40);
  assert(fan.update(28.01f, true, 1014) == 40);
  assert(fan.update(28, true, 1015) == 0);
  assert(fan.update(47, true, 1016) == 100);
  assert(fan.update(20, true, 1017) == 0);
}

static void failuresFreshnessAndRecovery() {
  Sample sample;
  FanPolicy fan;
  assert(sample.health(0) == SensorHealth::Waiting);
  assert(fan.update(0, false, 0) == 100);
  assert(sample.record(20, 50, 0));
  assert(sample.health(0) == SensorHealth::Healthy);
  assert(fan.update(sample.temperature(), true, 0) == 0);
  assert(sample.health(29999) == SensorHealth::Healthy);
  assert(sample.health(30000) == SensorHealth::Stale);
  assert(fan.update(sample.temperature(), sample.health(30000) == SensorHealth::Healthy, 30000) == 100);
  sample.fail();
  assert(sample.health(30001) == SensorHealth::Failed);
  assert(sample.age(30001) == 30001);
  assert(sample.record(35, 40, 30002));
  assert(fan.update(sample.temperature(), true, 30002) == 60);
  sample.fail();
  assert(sample.health(30003) == SensorHealth::Failed);
  assert(fan.update(sample.temperature(), false, 30003) == 100);
  assert(sample.record(20, 45, 30004));
  assert(fan.update(sample.temperature(), true, 30004) == 0);

  const float nan = std::numeric_limits<float>::quiet_NaN();
  const float infinity = std::numeric_limits<float>::infinity();
  const float badTemperatures[] = {nan, infinity, -40.01f, 85.01f, 255};
  for (float temperature : badTemperatures) {
    assert(!sample.record(temperature, 50, 40000));
    assert(sample.health(40000) == SensorHealth::Failed);
    assert(fan.update(temperature, false, 40000) == 100);
  }
  const float badHumidities[] = {nan, infinity, -0.01f, 100.01f, 255};
  for (float humidity : badHumidities) {
    assert(!sample.record(30, humidity, 40000));
    assert(sample.health(40000) == SensorHealth::Failed);
  }
  assert(fan.update(nan, true, 40001) == 100);
  sample.reset(40002);
  assert(sample.health(40002) == SensorHealth::Waiting);
  assert(!sample.hasReading());
  assert(sample.health(70001) == SensorHealth::Waiting);
  assert(sample.health(70002) == SensorHealth::Stale);
}

static void restartAndTimerWrap() {
  FanPolicy fan;
  const uint32_t start = UINT32_MAX - 500;
  assert(fan.update(35, true, start) == 100);
  assert(fan.update(35, true, uint32_t(start + 999)) == 100);
  assert(fan.update(35, true, uint32_t(start + 1000)) == 60);
  fan.reset();
  assert(fan.update(35, true, 10000) == 100);
  assert(fan.update(35, true, 11000) == 60);
  assert(fan.update(20, true, 11001) == 0);
  assert(fan.update(30, true, 11002) == 100);
  assert(fan.update(20, true, 11003) == 0);

  Sample sample;
  assert(sample.record(35, 50, start));
  assert(sample.health(uint32_t(start + 29999)) == SensorHealth::Healthy);
  assert(sample.health(uint32_t(start + 30000)) == SensorHealth::Stale);
}

static void physicalIndicatorMapping() {
  auto frame = rev_b::indicators(true, SensorHealth::Healthy, 0, 16);
  assert(frame.power == 0x001000);
  assert(frame.wifi == 0x000010);
  assert(frame.status == 0);
  frame = rev_b::indicators(false, SensorHealth::Waiting, 0, 16);
  assert(frame.power == 0x001000);
  assert(frame.wifi == 0x100800);
  assert(frame.status == 0x100800);
  frame = rev_b::indicators(false, SensorHealth::Failed, 500, 16);
  assert(frame.power == 0x001000);
  assert(frame.wifi == 0);
  assert(frame.status == 0x100000);
  frame = rev_b::indicators(true, SensorHealth::Stale, 1000, 16);
  assert(frame.power == 0x001000);
  assert(frame.wifi == 0x000010);
  assert(frame.status == 0x100000);
}

int main() {
  curveAndHysteresis();
  failuresFreshnessAndRecovery();
  restartAndTimerWrap();
  physicalIndicatorMapping();
  std::puts("Rev B policy checks passed.");
}
