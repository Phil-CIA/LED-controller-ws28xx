# My LED Controller Handoff

## Closeout summary (2026-06-24)
This phase is closed out for board-rev learning. We have enough validated data from PCB bring-up to proceed to next board revisions.

Current target identity state:
- Breadboard target: COM5, MAC `14:33:5C:2F:27:C4`
- PCB target: COM6, MAC `6C:C8:40:06:EE:54`
- Guarded upload remains fail-closed on expected port + chip + pinned MAC

## What was proven on PCB
- Guarded build/upload for `pcb` target succeeds repeatedly on COM6.
- External strip path works on GPIO18.
- On-board 3 LEDs work on GPIO19 after board-level trace/jumper rework.
- AHT20 is present and detected on I2C bus:
  - SDA: GPIO21
  - SCL: GPIO22
  - I2C scan found device at `0x38`.

## Firmware/code changes recorded in this phase
- Added dual-strip support:
  - external strip on GPIO18 (`LED_COUNT=300`)
  - onboard strip on GPIO19 (`ONBOARD_LED_COUNT=3`)
- Added board diagnostics in serial commands:
  - `aht20` / `sensor`
  - `i2cscan` / `scan`
  - `switches` / `inputs`
  - `boardcheck` / `diag`
- Added AHT20 support via `Adafruit AHTX0` + `Wire`.
- Added switch event logging with debounce in firmware.
- Updated `platformio.ini` build flags:
  - `AHT20_SDA_PIN=21`
  - `AHT20_SCL_PIN=22`
  - switch pins: GPIO34/35/36/39

Primary files changed:
- `src/main.cpp`
- `platformio.ini`

## Open issue (important for next board rev)
Switches are not reliable as currently wired because GPIO34/35/36/39 are input-only and do not provide internal pull-up/pull-down.

Observed behavior:
- Sensor path is confirmed.
- Button press reporting is not robust without proper bias network.

Root cause:
- Tactile switches on GPIO34-39 require external pull resistors.
- With no pull path, inputs float and state transitions are not deterministic.

## Next board revision requirements
- Add explicit biasing for each switch input (recommended):
  - 10k pull-up to 3.3V on each switch line
  - button to GND (active-low)
- Keep switch lines on GPIO34/35/36/39 only if external pulls are on PCB.
- Alternative: move switches to GPIOs supporting `INPUT_PULLUP` if you want to remove external resistors.

## Known-good command set for bring-up
- Build target:
  - `powershell -ExecutionPolicy Bypass -File scripts/guarded-flash.ps1 -Action build -TargetName pcb`
- Upload target (guarded):
  - `powershell -ExecutionPolicy Bypass -File scripts/guarded-flash.ps1 -Action upload -TargetName pcb`
- Monitor:
  - `C:/Users/user/.platformio/penv/Scripts/platformio.exe device monitor -b 115200 -p COM6`
- I2C diagnostics:
  - `i2cscan`
  - `aht20`
  - `boardcheck`

## Operational notes
- If upload fails with COM port access denied, close serial monitor first.
- Do not bypass guarded flash prechecks.
- Do not reuse breadboard MAC for PCB target.

## Ready-for-next-session checklist
- [x] PCB identity pinned and guarded upload validated
- [x] Strip output validated
- [x] Onboard LED path validated
- [x] AHT20 bus and address validated (`0x38`)
- [ ] Switch hardware biasing fixed in next PCB revision
