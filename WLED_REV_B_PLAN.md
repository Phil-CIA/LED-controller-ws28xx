# WLED Rev B integration plan

## Goal

Keep WLED and its app as the primary controller. Reuse built-in features and
existing usermods before adding board-specific code. The owner separately
approved the initial OTA upload recorded below; this plan is not blanket
authorization for further flashing, output changes or fan hardware tests.

## Current board information

Reported by the owner from the running WLED device:

| Function | GPIO | Details |
|---|---:|---|
| Main LEDs | 16 | WLED output configured for 50 LEDs |
| Onboard LEDs | 12 | WLED output configured for three LEDs |
| Brightness down | 34 | Button |
| Brightness up | 25 | Button |
| Mode | 36 | Button |
| Power | 39 | Button |
| Fan control | 23 | Fan circuit and control type still to verify |

The owner wants the onboard indicators to remain active when the main LEDs
are off: D1 for controller power, D2 for Wi-Fi, and D3 for a general fault or
warning. D3 should indicate a missing or failed temperature sensor. The owner
confirmed the physical LED data order is D1, D2, then D3, and that the board's
power/status silkscreens are reversed. Follow the component references and
chain order, not those two printed labels.
These reported pins supersede older pin assumptions for planning, but must be
checked against the physical board before hardware changes.

## Bring-up progress (2026-11-18)

- Read the running controller's WLED version: 16.0.1 on ESP32. The running
  device identity differs from the old guarded PCB flash profile. Do not
  change that profile or flash until the target is positively identified.
- Backed up the existing WLED configuration and presets in private session
  storage before changing button settings. The backups are not in the repo;
  separately confirm recovery of Wi-Fi credentials before any reflash.
- Confirmed both LED outputs and left their configuration and existing
  12,200 mA WLED current limit unchanged. That setting is not a new assessment
  of the power supply or wiring.
- The owner confirmed external button pull-ups and an AHT20 on GPIO21 (SDA)
  and GPIO22 (SCL). Electrical values and sensor readings remain unverified
  in WLED.
- Configured active-low pushbuttons and presets 1-4 for brightness down
  (`A=~-10`), brightness up (`A=~10`), next effect (`FX=~`), and power
  toggle (`T=2`). Kept the existing GPIO0 recovery button.
- The + button, mode button and power button produced the expected WLED state
  changes during physical testing. The - preset worked when invoked directly;
  physical tests were initially inconsistent. The owner confirmed GPIO34 goes
  low when pressed, returns to approximately 3.3 V when released, and that
  the - button works after the settings were restored.
- After an owner-initiated restart, the button assignments initially reverted
  while the presets remained. Restored the assignments, verified them in
  WLED's saved configuration, and confirmed they survived a second restart.
- The installed build has AudioReactive but it is disabled. Neither AHT20 nor
  independent indicator behavior is configured yet.
- The WLED 16.0.1 PWM fan usermod requires the Dallas Temperature or SHT
  usermod at compile time; it does not directly consume the AHT10/AHT20
  usermod. Fan integration therefore needs an adapter or another approach,
  after the fan hardware and safe behavior are verified.
- The owner plans to use 5 V, 12 V, or 24 V board supplies and select a fan
  rated for the supply. The historical Rev B netlist shows a two-pin fan
  connector with one side on `VIN_CLAMPED` and the other switched to ground
  by GPIO23 through Q2. Verify the assembled board and fan before use: PWM
  switches the supply and does not reduce its peak voltage. The netlist
  identifies Q2 as an AO3400A rated at 30 V; check voltage transients and
  driver margins, especially with a 24 V supply.

## Offline firmware implementation

`wled\usermods\RevBController` implements shared AHT20 health, the agreed
fan curve and independent status pixels without changing WLED core.
All three functions default **disabled**. Building does not change the
running controller, its outputs, buttons, presets or flash-target profiles.

The stock PWM fan usermod only accepts Dallas/SHT readings; the stock AHT10
usermod keeps its last successful reading and timestamp private. Rather than
coupling to its debug JSON or maintaining patches to both usermods, the
board module reuses the maintained AHTxx sensor library (pinned in
`library.json`), including AHT20 CRC and busy/error checks. It uses WLED's
GPIO/LEDC allocation APIs for the same kind of PWM output as the stock fan
usermod. It must not be built alongside AHT10 or PWM_fan.

The indicator driver uses WLED's existing NeoPixelBus dependency, with a
software output for just three pixels. This avoids taking an untracked
RMT/I2S channel that WLED could reuse when its outputs are reconfigured.
GPIO12 is exclusively allocated through WLED's pin manager; an existing
output on that pin is reported as a conflict, never removed automatically.
The indicator output uses GRB byte order, matching WLED v16.0.1's
color-order value 0 in the current GPIO12 configuration, and is outside
WLED's effects, master brightness and current limiter. Default channel
brightness is 16/255, with a configurable cap of 64.

| Component | Initial display after enabling indicators |
| --- | --- |
| D1 power | Green whenever the module is running |
| D2 Wi-Fi | Blue when connected; blinking amber when disconnected/AP-only |
| D3 status | Amber before the first reading; off when healthy; red on disabled, failed or stale sensor, or fan initialization failure |

These are firmware color defaults, not hardware-verified colors. They can
only be displayed after firmware starts and while the pixels have power.

### Build the combined firmware (no upload)

Use PlatformIO and Node.js/npm on the build machine, and a **separate** WLED
checkout pinned to v16.0.1, commit
`29b389df1c1aaec6ff53aea742d17063b985906c`.

1. Copy `wled\platformio_override.ini` into the WLED checkout root. Preserve
   any unrelated existing override rather than overwriting it.
2. Copy the complete `wled\usermods\RevBController` directory into that
   checkout's `usermods` directory.
3. From the WLED checkout, run `platformio run -e rev_b_controller`.
4. The application image is `.pio\build\rev_b_controller\firmware.bin`.
   It is not a merged factory image. A successful build does not verify the
   connected ESP32's identity, partition layout or configuration compatibility.

The combined profile built successfully on 2026-11-19: 1,304,673 bytes of
program flash in the selected 1,572,864-byte application partition, and
81,640 bytes of static RAM. A local copy of that 1,311,360-byte application
image is `.pio\wled-revb\WLED_16.0.1_RevBController.bin` in this project
workspace (ignored by Git). Its SHA-256 is
`688069779f9b9a035713644eb92b0ee9759c531a2483ba6f0d4bf1337004b755`.
These are build results, not compatibility or hardware-test results.

The combined profile retains AudioReactive but defaults its input pins/type
to unassigned. Existing saved settings still take precedence; do not enable
the microphone until its interface and supply have been verified.

### Initial OTA deployment (2026-11-19)

The owner requested flashing and confirmed USB access and Wi-Fi credentials
for recovery. Matched the saved `wled-c9df3c` device identity to the live
classic ESP32, MAC `78:1C:3C:C9:DF:3C`, with 4 MB flash and the `ESP32`
release. Saved fresh private configuration, presets, runtime state and info
backups. Downloaded the matching official recovery application image and
verified its release SHA-256; this is not a dump of the original flash.

Uploaded the application image identified above through WLED's `/update`
endpoint, with firmware validation enabled. WLED reported a successful
update and rebooted. No raw-address writes, erase, partition/bootloader
changes, USB profile changes or validation bypass were used.

Confirmed the same MAC after reboot, WLED 16.0.1, the new Rev B Info entries,
increasing uptime, and saved RevBController settings with all three
functions disabled. Both original LED outputs, all button assignments and
presets 1-4 matched the preflash backup. This checks saved configuration and
network access, not a new physical button or cooling test.

GPIO12 remains an ordinary WLED output. Sensor/indicator activation and
the GPIO12 migration below have not been performed. The fan and microphone
remain disabled; fan PWM frequency is still unset.

### Future activation and rollback

The initial upload does not authorize fan hardware tests. Before any further
separately approved upload, positively identify the ESP32 and its flash/OTA
partition layout, verify the image fits,
save current configuration and presets privately, and separately establish
recovery of Wi-Fi credentials and a known-good firmware image. The old
`pcb` and `breadboard` identity profiles are not verified for this controller.

The installed firmware's WLED Usermods page exposes:

| RevBController setting | Default | Meaning |
| --- | --- | --- |
| `sensor-enabled` | false | Read AHT20 at `0x38` using WLED's I2C bus |
| `indicators-enabled` | false | Own the three GPIO12 pixels |
| `fan-enabled` | false | Enable automatic GPIO23 PWM |
| `indicator-brightness` | 16 | Per-channel brightness, accepted range 0-64 |
| `fan-pwm-Hz` | 0 | Unset: fan refuses to start until a verified frequency between 20 and 25000 Hz is supplied |

For sensor activation, set WLED's I2C SDA/SCL to GPIO21/22 and reboot before
enabling `sensor-enabled`. The module only accesses an already initialized
WLED-owned bus on those pins; it does not reassign pins or restart Wire.
It retries failed sensor initialization/readings every five seconds.
The reused sensor library performs bounded blocking conversions (about 80 ms
normally), so animation responsiveness during readings still needs a bench
check. Missed reads during strip updates eventually trigger the stale
fail-safe rather than treating an old sample as fresh.

For indicator migration, record the existing GPIO12 output (three WS2812
pixels, start 50, GRB/color-order value 0) and segment/preset configuration
first. Keep GPIO16 and its 50 LEDs unchanged. With indicators still disabled,
remove only the ordinary GPIO12 output, save and reboot. Then enable
`indicators-enabled` and save. Check the WLED Info panel for pin conflicts
and health; never configure another GPIO12 output while the module owns it.
Main segments should cover only LEDs 0-49; review saved presets that used
the former 53-pixel layout.

Leave the fan disabled until its voltage, pin functions and power-PWM
compatibility are verified and a bench session is approved. No PWM frequency
is assumed for the three-wire example. The module reserves a whole LEDC
timer pair and reports pin, timer or frequency setup failures in WLED Info.
If enabled successfully, it commands full power until a valid reading is
available, on failed/stale/disabled sensor data, and during firmware updates.
Disabling the module's fan setting explicitly stops its output; this is not
a fail-safe and must not be used while relying on the fan for cooling.
There is no tachometer feedback or stall detection.

For rollback, disable `fan-enabled` and `indicators-enabled`, save, and reboot
to release their pins. Recreate only the original GPIO12 output and restore
the saved segment/preset layout if needed. If firmware rollback is necessary,
use the previously verified compatible image and recovery method, then
restore the private configuration/presets. A full erase requires a separate
credential recovery path; never assume the application `.bin` is suitable
for an arbitrary flash address.

### Offline policy checks

From this repository root, compile and run
`wled\tests\rev_b_policy_test.cpp` with a host C++11 compiler; these checks
do not communicate with a board. For example, with Zig installed:

```powershell
zig c++ -std=c++11 -Wall -Wextra -Werror wled\tests\rev_b_policy_test.cpp -o "$env:TEMP\rev_b_policy_test.exe"
if ($LASTEXITCODE -eq 0) { & "$env:TEMP\rev_b_policy_test.exe" }
```

The checks cover the actual curve boundaries, downward hysteresis, start
kick, failed/invalid/stale data, recovery, millisecond rollover and the
D1/D2/D3 mapping.

The second executable tests the actual usermod against simulated pins, PWM,
pixels and sensor responses. It uses the pinned WLED checkout's bundled
ArduinoJson header:

```powershell
$wled = "C:\wled-revb" # Your separate pinned WLED checkout.
zig c++ -std=c++11 -Wall -Wextra -Werror -I wled\tests\fakes -I "$wled\wled00\src\dependencies\json" wled\tests\rev_b_controller_test.cpp -o "$env:TEMP\rev_b_controller_test.exe"
if ($LASTEXITCODE -eq 0) { & "$env:TEMP\rev_b_controller_test.exe" }
```

Both offline executables passed on 2026-11-19, including disabled defaults,
GPIO conflicts, allocation failures, initialization/read failures, stale
data during strip updates, recovery, update fail-safe and pin release.
No actual device is involved in these checks. Hardware validation remains
deferred.

## Earlier AHT20-only build profile

`wled\platformio_override.ini` adds the upstream AHT10/AHT20 usermod to
WLED's `esp32dev` build while retaining its audio-reactive usermod. It does
not include the independent status indicators or fan control.

Build against WLED **v16.0.1**, commit
`29b389df1c1aaec6ff53aea742d17063b985906c`, in a separate WLED checkout.
PlatformIO and Node.js/npm must be available on the build machine.

1. Copy `wled\platformio_override.ini` from this repository into that
   checkout as `platformio_override.ini`.
2. From the WLED checkout, run `platformio run -e rev_b_aht20`.
3. Before any upload, confirm the actual ESP32 flash layout and board
   identity, keep the backup and recovery path available, and obtain approval.

After an approved upload, configure WLED's I2C SDA/SCL as GPIO21/22 and the
AHTxx usermod for AHT20 at address 56 (`0x38`). Confirm live readings and
error behavior before relying on them. Do not enable the microphone without
first verifying its type and supply.

The profile built successfully against the pinned source with both AHT10
and AudioReactive included. It has not been flashed or tested on the board;
the build alone does not establish flash-layout or configuration compatibility.

## 1. Capture the working baseline

- Record the installed WLED version and build, the ESP32 type, the actual LED
  types and order, output settings, and button wiring and polarity.
- Back up WLED configuration and presets using WLED's backup functions; keep
  backups containing Wi-Fi credentials out of the repository.
- Verify the AHT20 and microphone models, interfaces, pins, and power rails;
  determine whether the microphone is populated.
- Verify the GPIO23 fan driver, fan type and supply, safe control method, and
  whether the temperature sensor measures the component that needs cooling.
- Record a recovery path and confirm target identity before any flash. Do not
  erase or reflash the working device as part of baseline capture.

## 2. Configure and verify WLED's built-in controls

- Preserve the existing GPIO16 output and its 50-LED setting.
- Check that the four buttons have reliable electrical biasing. On a classic
  ESP32, GPIO34, GPIO36 and GPIO39 have no internal pull-ups; confirm external
  pull-ups and voltage compatibility before relying on them.
- Configure the button pins and use WLED presets/macros for brightness down,
  brightness up, mode and power. WLED button 0 has long-hold access
  point and factory-reset functions; take care during testing.
- Validate app control and each button repeatedly without breaking the main
  output or changing its current-limit configuration.

## 3. Add independent onboard status indicators

- Check existing WLED status usermods and supported LED-driver approaches.
  The located Wi-Fi status example drives separate GPIO LED channels rather
  than the three addressable pixels, so it is not a drop-in solution.
- Design a small WLED usermod only if no suitable existing solution is found.
  Establish exclusive ownership of GPIO12: WLED's ordinary LED output and the
  usermod must not drive the same pin simultaneously. Define the migration
  and rollback of the existing three-pixel output before changing it.
- D1 indicates controller operation after the firmware starts; D2 indicates
  Wi-Fi connection state; D3 indicates a missing or failed temperature sensor.
  Agree on colors and define sensible startup and fault behavior.
- Validate all three indicators across boot, Wi-Fi disconnect/reconnect, main
  LED on/off, brightness changes, effects and a simulated agreed warning.
  Verify that GPIO12's ESP32 boot-strapping role does not prevent reliable
  startup. Indicators cannot report status when the board is unpowered.

## 4. Integrate sensors and fan

- Evaluate the upstream AHT10 usermod with its AHT20 option, and include it
  in a compatible custom WLED build if the installed firmware lacks it.
- Evaluate a fan control approach against the verified fan circuitry. The
  upstream PWM fan usermod does not directly consume AHT20 readings.
- Agree on safe fan thresholds and a fail-safe for missing or stale sensor
  readings before enabling automatic control. Bench-test the driver and
  failure behavior before relying on it for cooling.
- For the microphone, verify the actual device and interface before selecting
  WLED AudioReactive settings. Earlier Rev B notes describe an INMP441 I2S
  microphone and a possible 5 V supply; the bare INMP441 requires a suitable
  low-voltage supply. Do not enable or connect it until the module and rail
  are confirmed safe.

### Agreed provisional fan behavior

The owner accepted the following starting settings and deferred hardware
testing. They are implemented in the opt-in board module, but are not
validated cooling limits. Do not enable fan control or run fan hardware tests
until a separate bench session is approved and the fan is verified.

| AHT20 temperature | Fan duty |
| --- | --- |
| Below 30 C | Off |
| 30 C to below 35 C | 40% |
| 35 C to below 40 C | 60% |
| 40 C to below 45 C | 80% |
| 45 C or higher | 100% |

Read the sensor every five seconds. Require a 2 C drop below a boundary
before stepping down across it. When starting from off, command full power
for one second before applying the requested duty. Missing or failed readings,
or no fresh reading for 30 seconds, command 100%. Fan control must remain
independent of main LED power and brightness.

The board module uses AHT20 readings with explicit validity and freshness
information rather than the stock fan usermod's Dallas/SHT interface. The
owner reports a three-wire fan;
its exact pin functions, supply-PWM compatibility, PWM frequency, minimum
reliable duty, and the sensor's placement remain to be verified before use.

## 5. Build, deploy and validate

- Select and pin a compatible WLED release and document the usermods and build
  instructions separately from this repository's existing custom firmware.
- Build and check the image before requesting approval for a guarded,
  identity-verified upload. Preserve a documented recovery path.
- After approved deployment, verify app access, both LED paths, all buttons,
  independent indicators, sensor readings, fan behavior and any enabled
  audio feature. Record results and update the handoff and tracker.

## References

- [WLED multiple LED outputs](https://kno.wled.ge/features/multi-strip/)
- [WLED buttons and macros](https://kno.wled.ge/features/macros/)
- [WLED custom features and usermods](https://kno.wled.ge/advanced/custom-features/)
- [AHT10/AHT20 usermod](https://github.com/wled/WLED/tree/main/usermods/AHT10_v2)
- [PWM fan usermod](https://github.com/wled/WLED/tree/main/usermods/PWM_fan)
- [Wi-Fi status LED usermod example](https://github.com/ptank1985-qvh/wled-wifi-status-led-usermod)
- [WLED audio reactive](https://kno.wled.ge/advanced/audio-reactive/)
