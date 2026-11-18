# WLED Rev B integration plan

## Goal

Keep WLED and its app as the primary controller. Reuse built-in features and
existing usermods before adding board-specific code. This plan does not
authorize flashing or changing the running board.

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
warning. D3 should indicate a missing or failed temperature sensor; indicator
colors are not yet decided. The owner confirmed the physical LED data order
is D1, D2, then D3.
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

## WLED AHT20 build profile

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
