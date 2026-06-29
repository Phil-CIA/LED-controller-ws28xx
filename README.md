# My LED Controller

Standalone ESP32 + WS281x project.

This repo now supports two firmware paths:
- custom firmware in `src/main.cpp` (serial-driven tests and demo)
- official WLED firmware (Wi-Fi + WLED app control)

This repo now also supports two hardware targets:
- `breadboard` target profile
- `pcb` target profile

Each target has independent identity lock (port/chip/MAC) in `scripts/flash-targets.json`.

## Hardware
- MCU: ESP32 family (default board config: esp32dev)
- LED data: GPIO18
- LED type: WS281x (NEO_GRB + 800kHz)
- LED count: 300
- Level shifter: 74H125
- Upload port: COM5

## Quick start
1. Set active target profile
   - Breadboard: `powershell -ExecutionPolicy Bypass -File scripts/set-active-target.ps1 -TargetName breadboard`
   - PCB: `powershell -ExecutionPolicy Bypass -File scripts/set-active-target.ps1 -TargetName pcb`
2. Build custom firmware for selected target
   - `powershell -ExecutionPolicy Bypass -File scripts/guarded-flash.ps1 -Action build`
3. Upload custom firmware for selected target (guarded)
   - `powershell -ExecutionPolicy Bypass -File scripts/guarded-flash.ps1 -Action upload`
4. Monitor
   - `platformio device monitor -b 115200 -p COM5`

## Target profile setup
Before first guarded flash on a target, pin its immutable MAC:
- `powershell -ExecutionPolicy Bypass -File scripts/set-target-mac.ps1 -TargetName pcb -ExpectedMac 11:22:33:44:55:66`

If `expectedMac` is empty for a target, guarded upload refuses to flash.

## WLED quick start (Wi-Fi + app)
1. Set active target profile first (`breadboard` or `pcb`).
2. Use the official installer at `https://install.wled.me` (ESP32 target).
3. Select the target COM port and complete flashing through the installer UI.
4. On first boot, connect to the WLED access point (`WLED-xxxxxx`) and open `http://4.3.2.1`.
5. In WLED settings, set hardware values to match this strip:
   - GPIO: `18`
   - LED count: `300`
   - Color order: `GRB`
6. Configure home Wi-Fi credentials in WLED settings and save.
7. Open the WLED app and add the device using `wled.local` or the assigned IP.
8. Set WLED max current to `12000 mA` for steady-state operation.
9. Discover by target profile:
   - Breadboard: `powershell -ExecutionPolicy Bypass -File scripts/discover-wled.ps1 -TargetName breadboard -OpenUi`
   - PCB: `powershell -ExecutionPolicy Bypass -File scripts/discover-wled.ps1 -TargetName pcb -OpenUi`

### If device reboots with `invalid header`
- This indicates an incompatible direct flash layout.
- Recover to known-good firmware with guarded upload:
  - `powershell -ExecutionPolicy Bypass -File scripts/guarded-flash.ps1 -Action upload`
- Then use `install.wled.me` instead of raw one-file `esptool write-flash` for WLED.

## Serial commands
- `on` => resume animation
- `off` => LEDs off
- `rainbow` => return to rainbow mode
- `white`, `red`, `green`, `blue` => solid color test
- `probe` => raw GPIO18 pulse test
- `b0` to `b255` => set brightness

## Rev B action list (from validated PCB bring-up)
- Keep LED outputs as validated:
   - external strip data on `GPIO18`
   - onboard 3-LED chain on `GPIO19`
- Keep sensor wiring as validated:
   - AHT20 `SDA=GPIO21`, `SCL=GPIO22`
   - I2C address detected at `0x38`
- Fix switch hardware biasing on PCB:
   - Current switch pins (`GPIO34`, `GPIO35`, `GPIO36`, `GPIO39`) have no internal pull-up/down support.
   - Add external pull resistors per switch (recommended: `10k` pull-up to `3.3V`, button to `GND`, active-low).
- Preserve guarded flash identity lock per target in `scripts/flash-targets.json`.
- Keep target-separated profiles (`breadboard`, `pcb`) and avoid MAC reuse across targets.

## PCB diagnostics commands
- `aht20` or `sensor` => read temperature/humidity from AHT20
- `i2cscan` or `scan` => scan bus and print detected I2C addresses
- `switches` or `inputs` => print raw switch pin states
- `boardcheck` or `diag` => run sensor + switches diagnostics together

## Notes
- If your exact ESP32 board is different, update `board` in platformio.ini.
- If colors are wrong, adjust pixel order in src/main.cpp.
- Keep strict target guardrails in place; do not flash unknown devices opportunistically.
- Use dedicated VS Code tasks per target (`Breadboard:*` and `PCB:*`).
