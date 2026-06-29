# Next Chat Quick Start - Rev B Pre-Route Execution

Primary tracker document: `REV_B_CHANGE_TRACKER.md` (living document, continuously updated)

## Goal for next chat
Complete the Rev B schematic pre-route gate, then start the first must-have board changes.

## Paste this at the start of the next chat
Use this workspace to execute Rev B pre-route gates first, then begin layout must-haves.

Known validated state:
- PCB target identity locked and guarded upload working on COM6
- External strip path validated on GPIO18
- Onboard 3-LED path validated on GPIO19
- AHT20 validated on I2C: SDA GPIO21, SCL GPIO22, device at address 0x38
- Switches currently unreliable due to hardware biasing limits on GPIO34/35/36/39

Pre-route findings already identified:
- Rev B mic path is implemented as U2 INMP441 with Set A GPIO mapping.
- One true ERC blocker remains on GPIO33 (input-not-driven).
- Mic VDD decision is still open (+5V_CTRL vs +3.3V).
- Mic naming/docs mismatch is still open.

Please execute in this order:
1. Close the GPIO33 ERC blocker (fix or explicit waiver with rationale).
2. Lock mic VDD rail decision and apply schematic update if needed.
3. Resolve mic naming/docs mismatch and ensure optional or DNI intent is explicit.
4. Regenerate Rev B ERC and DRC artifacts.
5. If gate passes, start RB-001 and RB-007 before broader reroute.

Then update:
- REV_B_CHANGE_TRACKER status fields for completed pre-route items.
- HANDOFF with exact decisions and artifact filenames.

## Current source of truth
- HANDOFF: HANDOFF.md
- Pre-route execution plan: REV_B_MERGED_DIRECTION_2026-06-29.md
- Rev B prefab checklist: Rev B/PREFAB_SCHEMATIC_CHECKLIST_2026-06-29.md
- Main firmware: src/main.cpp
- Build config and pins: platformio.ini
- Guarded flash flow: scripts/guarded-flash.ps1
- Target identity config: scripts/flash-targets.json

## First implementation block (after pre-route gate)
### Hardware and layout must-haves
- RB-001: add external pull-up network for switch lines GPIO34/35/36/39.
- RB-007: correct onboard LED serial chain first-LED orientation (DIN path).
- RB-002: route and place switch pull network near MCU pins.
- RB-003/RB-005: onboard LED section and R16 relocation work.

## Fast command reference
- KiCad ERC (Rev B schematic):
  C:/Users/forch/AppData/Local/Programs/KiCad/9.0/bin/kicad-cli.exe sch erc "c:/Users/forch/GitHub/LED-controller-ws28xx/Rev B/My LED controller Rev B.kicad_sch" --output "c:/Users/forch/GitHub/LED-controller-ws28xx/Rev B/ERC_NEXT_SESSION_YYYY-MM-DD.rpt"

- KiCad DRC (Rev B PCB):
  C:/Users/forch/AppData/Local/Programs/KiCad/9.0/bin/kicad-cli.exe pcb drc "c:/Users/forch/GitHub/LED-controller-ws28xx/Rev B/My LED controller Rev B.kicad_pcb" --output "c:/Users/forch/GitHub/LED-controller-ws28xx/Rev B/DRC_NEXT_SESSION_YYYY-MM-DD.rpt"

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
