# Rev B Handoff Note

Date: 2026-06-26
Repo: LED-controller-ws28xx

## Purpose
This folder snapshots the current KiCad design state as a Rev B working baseline for upcoming schematic/PCB changes.

## Source Snapshot
Files were copied from the repository root and renamed with "Rev B":
- My LED controller.kicad_sch -> My LED controller Rev B.kicad_sch
- My LED controller.kicad_pcb -> My LED controller Rev B.kicad_pcb
- My LED controller.kicad_pro -> My LED controller Rev B.kicad_pro
- My LED controller.kicad_prl -> My LED controller Rev B.kicad_prl

## Current Focus
Primary validation objective carried from prior handoff:
- Verify PMOS reverse-polarity gate-bias fix (R5/R6 swap) on hardware.
- Confirm VIN_REVPROT rises toward 4.9-5.0V at 5V input.

## Next Steps
1. Open the Rev B project files in KiCad for all new edits.
2. Keep Rev B changes isolated from root-level files.
3. Record bench measurements and update handoff notes after validation.

## Microphone Provisioning Implementation (Rev B)

Objective: add optional microphone support for WLED without populating a microphone by default.

### Provisioning Scope
- Interface: I2S digital microphone path (primary WLED-compatible option).
- Build intent: header-only provisioning, no mandatory mic population.
- Power intent: expose 3V3 and GND at mic header.
- Feature intent: keep this optional and non-blocking for primary LED controller operation.

### Provisional Net Plan
- MIC_3V3
- MIC_GND
- MIC_I2S_BCLK
- MIC_I2S_WS
- MIC_I2S_SD

### Locked GPIO Reservation
- Set A locked: GPIO32 (BCLK), GPIO33 (WS), GPIO35 (SD, input).

### Locked Module and Board Interface
- Module lock: INMP441 I2S microphone module, 14 mm, dual 1x3 headers.
- Current implementation reference: U2.
- KiCad symbol/library: 1My_IC's:INMP441.
- KiCad footprint/library: 1My_Parts_ ICs:inmp441.
- Locked U2 pin mapping:
	1) L/R -> GND (left-channel select)
	2) WS -> MIC_I2S_WS
	3) SCLK -> MIC_I2S_BCLK
	4) SD -> MIC_I2S_SD
	5) VDD -> +5V_CTRL
	6) GND -> GND

Notes:
- Keep SD on an input-capable pin for clean I2S receive mapping.
- Keep the entire microphone path optional and DNI by default.

### KiCad Edit Checklist
1. In the Rev B schematic, add an unpopulated microphone header symbol labeled MIC_I2S.
2. Label pins with the provisional net plan above.
3. Add schematic note: "Optional feature, DNI by default".
4. In the Rev B PCB, place the MIC_I2S footprint in a low-noise area away from high-current switching paths.
5. Route or prepare short, clean breakout routing for MIC_I2S signals with clear silkscreen pin naming.
6. Run ERC/DRC and capture any new warnings before closing the change.

### Firmware Bring-Up Note (Future)
- WLED AudioReactive should be configured for I2S mic mode once hardware is populated.
- Record final GPIO assignment in handoff and pin map after schematic/PCB lock.

### Implementation Reference
- See `Rev B/MIC_I2S_PROVISIONING_SPEC.md` for the current Rev B microphone provisioning baseline.

## Pre-Fab Schematic Check (Started 2026-06-29)
- Checklist: `Rev B/PREFAB_SCHEMATIC_CHECKLIST_2026-06-29.md`
- ERC baseline report: `Rev B/ERC_PREFAB_2026-06-29.rpt`
- Current blocker to clear before schematic pass: GPIO33 path flagged as input-not-driven in ERC.

## Broadened Scope Audit (2026-06-29)
- Audit summary: `Rev B/PREFAB_SCOPE_AUDIT_2026-06-29.md`
- Root ERC comparison report: `ERC_ROOT_BASELINE_2026-06-29.rpt`
- Rev B DRC baseline: `Rev B/DRC_PREFAB_2026-06-29.rpt`
- Rev B local lib-table sync artifacts: `Rev B/sym-lib-table`, `Rev B/fp-lib-table`
- Rev B ERC after lib-table sync: `Rev B/ERC_PREFAB_2026-06-29_LIBTABLE_SYNC.rpt`

## Library Alias Pass (2026-06-29)
- Added Rev B project-local library aliases in `Rev B/sym-lib-table` and `Rev B/fp-lib-table`.
- Tuned Rev B-only library-related severities in `Rev B/My LED controller Rev B.kicad_pro` from warning to ignore for pre-route checks.
- ERC progression:
	- Baseline: `Rev B/ERC_PREFAB_2026-06-29.rpt` (48 messages)
	- Alias pass: `Rev B/ERC_PREFAB_2026-06-29_ALIAS_PASS.rpt` (44 messages)
	- Alias + severity pass: `Rev B/ERC_PREFAB_2026-06-29_ALIAS_PASS2.rpt` (35 messages)
- DRC progression:
	- Baseline: `Rev B/DRC_PREFAB_2026-06-29.rpt` (17 violations)
	- Alias + severity pass: `Rev B/DRC_PREFAB_2026-06-29_ALIAS_PASS2.rpt` (13 violations)
