#include "../usermods/RevBController/rev_b_controller.cpp"
#include <cstdio>

static void resetHardware() {
  fake::now = 1000;
  for (auto& owner : fake::owners) owner = PinOwner::None;
  for (auto& channel : fake::ledcUsed) channel = false;
  fake::owners[21] = fake::owners[22] = PinOwner::HW_I2C;
  fake::owners[16] = PinOwner::BusDigital;
  fake::ledcOk = fake::allocationOk = fake::sensorOk = fake::resetOk = true;
  fake::updating = false;
  fake::resetCalls = fake::readCalls = fake::pwmCalls = fake::frames = 0;
  fake::attachedPin = -1;
  fake::temperature = 35;
  fake::humidity = 50;
  i2c_sda = 21;
  i2c_scl = 22;
}

static void configure(RevBController& mod, bool sensor, bool indicators, bool fan,
                      int frequency = 1000, int brightness = 16) {
  StaticJsonDocument<512> document;
  JsonObject root = document.to<JsonObject>();
  JsonObject config = root.createNestedObject("RevBController");
  config["sensor-enabled"] = sensor;
  config["indicators-enabled"] = indicators;
  config["fan-enabled"] = fan;
  config["indicator-brightness"] = brightness;
  config["fan-pwm-Hz"] = frequency;
  assert(mod.readFromConfig(root));
}

static void defaultsAndConflicts() {
  resetHardware();
  {
    RevBController mod;
    mod.setup();
    mod.loop();
    assert(fake::resetCalls == 0 && fake::readCalls == 0);
    assert(fake::pwmCalls == 0 && fake::frames == 0);
    assert(fake::owners[12] == PinOwner::None && fake::owners[23] == PinOwner::None);
  }
  fake::owners[12] = PinOwner::BusDigital;
  fake::owners[23] = PinOwner::HW_SPI;
  {
    RevBController mod;
    configure(mod, false, true, true);
    mod.setup();
    mod.loop();
    StaticJsonDocument<1024> document;
    JsonObject root = document.to<JsonObject>();
    mod.addToJsonInfo(root);
    assert(strstr(root["u"]["Rev B indicators"][0].as<const char*>(), "GPIO12"));
    assert(strstr(root["u"]["Rev B fan"][0].as<const char*>(), "GPIO23"));
    assert(fake::pwmCalls == 0 && fake::frames == 0);
  }
  assert(fake::owners[12] == PinOwner::BusDigital && fake::owners[23] == PinOwner::HW_SPI);
  assert(fake::owners[16] == PinOwner::BusDigital);
}

static void initializationFailures() {
  resetHardware();
  {
    RevBController mod;
    configure(mod, false, true, true, 0, 65);
    mod.setup();
    mod.loop();
    assert(fake::owners[12] == PinOwner::None && fake::owners[23] == PinOwner::None);
    assert(fake::pwmCalls == 0 && fake::frames == 0);
  }
  for (auto& channel : fake::ledcUsed) channel = true;
  {
    RevBController mod;
    configure(mod, false, false, true);
    mod.setup();
    assert(fake::owners[23] == PinOwner::None);
    assert(fake::pwmCalls == 0);
  }
  for (auto& channel : fake::ledcUsed) channel = false;
  fake::ledcOk = false;
  fake::allocationOk = false;
  {
    RevBController mod;
    configure(mod, false, true, true);
    mod.setup();
    assert(fake::owners[12] == PinOwner::None && fake::owners[23] == PinOwner::None);
    assert(!fake::ledcUsed[0] && !fake::ledcUsed[1]);
    assert(fake::pwmCalls == 0 && fake::frames == 0);
  }
}

static void controlFailureRecoveryAndRelease() {
  resetHardware();
  {
    RevBController mod;
    configure(mod, true, true, true);
    mod.setup();
    assert(fake::pwm == 255);
    assert(fake::attachedPin == 23);
    assert(fake::ledcUsed[0] && fake::ledcUsed[1]);
    mod.loop();
    assert(fake::resetCalls == 1 && fake::readCalls == 1);
    assert(fake::colors[0] == 0x001000 && fake::colors[1] == 0x000010 && fake::colors[2] == 0);
    assert(fake::pwm == 255); // starting from off still receives the full one-second kick
    fake::now += 1000;
    mod.loop();
    assert(fake::pwm == 153);
    fake::updating = true;
    fake::now = 30999;
    mod.loop();
    assert(fake::pwm == 153 && fake::readCalls == 1);
    fake::now = 31000;
    mod.loop();
    assert(fake::pwm == 255 && fake::colors[2] == 0x100000);
    fake::updating = false;
    fake::sensorOk = false;
    mod.loop();
    assert(fake::pwm == 255 && fake::colors[2] == 0x100000);
    fake::now += 5000;
    fake::sensorOk = true;
    mod.loop();
    assert(fake::pwm == 153 && fake::colors[2] == 0);
    mod.onUpdateBegin(true);
    assert(fake::pwm == 255);
    mod.loop();
    assert(fake::pwm == 255);
    mod.onUpdateBegin(false);
    mod.loop();
    assert(fake::pwm == 153);
    i2c_sda = -1;
    mod.loop();
    assert(fake::pwm == 255 && fake::colors[2] == 0x100000);
    configure(mod, false, false, false);
    mod.loop();
    assert(fake::pwm == 0 && fake::attachedPin == -1);
    assert(fake::owners[12] == PinOwner::None && fake::owners[23] == PinOwner::None);
    assert(!fake::ledcUsed[0] && !fake::ledcUsed[1]);
    assert(fake::colors[0] == 0 && fake::colors[1] == 0 && fake::colors[2] == 0);
  }
  assert(fake::owners[16] == PinOwner::BusDigital);
  assert(fake::owners[21] == PinOwner::HW_I2C && fake::owners[22] == PinOwner::HW_I2C);
}

static void missingSensorAndFirstSampleTimeout() {
  resetHardware();
  RevBController mod;
  configure(mod, true, true, true);
  mod.setup();
  fake::updating = true;
  mod.loop();
  assert(fake::readCalls == 0 && fake::colors[2] == 0x100800);
  fake::now += 30000;
  mod.loop();
  assert(fake::pwm == 255 && fake::colors[2] == 0x100000);
  fake::updating = false;
  fake::resetOk = false;
  mod.loop();
  assert(fake::resetCalls == 1 && fake::readCalls == 0);
  assert(fake::pwm == 255 && fake::colors[2] == 0x100000);
  fake::now += 4999;
  mod.loop();
  assert(fake::resetCalls == 1);
  fake::now += 1;
  fake::resetOk = true;
  mod.loop();
  assert(fake::resetCalls == 2 && fake::readCalls == 1);
  assert(fake::pwm == 153 && fake::colors[2] == 0);
}

int main() {
  defaultsAndConflicts();
  initializationFailures();
  controlFailureRecoveryAndRelease();
  missingSensorAndFirstSampleTimeout();
  std::puts("Rev B integration checks passed (simulated hardware only).");
}
