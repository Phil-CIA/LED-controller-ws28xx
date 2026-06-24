# Next Chat Quick Start - PCB Change Categorization

Primary tracker document: `REV_B_CHANGE_TRACKER.md` (living document, continuously updated)

## Goal for next chat
Organize all PCB findings and firmware updates into clear change categories for the next board revision and implementation cycle.

## Paste this at the start of the next chat
Use this workspace to categorize PCB changes and produce a Rev B action plan.

Known validated state:
- PCB target identity locked and guarded upload working on COM6
- External strip path validated on GPIO18
- Onboard 3-LED path validated on GPIO19
- AHT20 validated on I2C: SDA GPIO21, SCL GPIO22, device at address 0x38
- Switches currently unreliable due to hardware biasing limits on GPIO34/35/36/39

Please categorize changes into:
1. Hardware schematic changes
2. PCB layout changes
3. Firmware changes
4. Manufacturing and assembly changes
5. Test and validation changes
6. Documentation updates

Then produce:
- Rev B must-have changes
- Rev B nice-to-have changes
- Risks if deferred
- Verification checklist per change category

## Current source of truth
- HANDOFF: HANDOFF.md
- Main firmware: src/main.cpp
- Build config and pins: platformio.ini
- Guarded flash flow: scripts/guarded-flash.ps1
- Target identity config: scripts/flash-targets.json

## Suggested categorization baseline

### 1) Hardware schematic changes
- Add external pull resistors for switch lines on GPIO34/35/36/39
- Confirm switch topology (recommended active-low: pull-up to 3.3V, button to GND)
- Preserve validated I2C wiring for AHT20 (SDA21/SCL22)

### 2) PCB layout changes
- Route and place switch pull networks near MCU input pins
- Keep proven LED data routing assumptions (GPIO18 external, GPIO19 onboard)
- Keep I2C trace integrity and pull-up strategy consistent with validated behavior

### 3) Firmware changes
- Keep dual-LED support and diagnostics commands
- Keep I2C scan and AHT20 read commands
- Keep switch logging logic, but expect true reliability only after hardware pull network is added

### 4) Manufacturing and assembly changes
- Include resistor values and references in BOM for switch biasing
- Add assembly notes for any trace/jumper ECOs discovered during bring-up

### 5) Test and validation changes
- Required bench test sequence:
  - guarded upload precheck pass
  - i2cscan finds 0x38
  - aht20 returns plausible readings
  - switch press logging confirms each switch
  - onboard and external LED functional tests

### 6) Documentation updates
- Update README Rev B section after schematic/layout freeze
- Update HANDOFF with final Rev B decisions and acceptance criteria

## Fast command reference
- Build PCB firmware:
  powershell -ExecutionPolicy Bypass -File scripts/guarded-flash.ps1 -Action build -TargetName pcb

- Upload PCB firmware (guarded):
  powershell -ExecutionPolicy Bypass -File scripts/guarded-flash.ps1 -Action upload -TargetName pcb

- Serial monitor:
  C:/Users/user/.platformio/penv/Scripts/platformio.exe device monitor -b 115200 -p COM6

- Diagnostics in serial:
  i2cscan
  aht20
  switches
  boardcheck
