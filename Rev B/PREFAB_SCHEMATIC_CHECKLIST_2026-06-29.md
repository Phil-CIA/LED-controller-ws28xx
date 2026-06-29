# Rev B Schematic Pre-Fab Checklist

Date: 2026-06-29
Design: Rev B
Scope: Schematic-only pre-fab gate before PCB reroute.

## Baseline Inputs
- Schematic: [Rev B/My LED controller Rev B.kicad_sch](Rev B/My LED controller Rev B.kicad_sch)
- Netlist: [Rev B/My LED controller Rev B.net](Rev B/My LED controller Rev B.net)
- ERC report (new): [Rev B/ERC_PREFAB_2026-06-29.rpt](Rev B/ERC_PREFAB_2026-06-29.rpt)

## Gate Criteria
1. ERC errors: 0 required to pass.
2. ERC warnings: either resolved or explicitly accepted with rationale.
3. New microphone path fully documented and electrically intentional.
4. All power-net assumptions for microphone module confirmed.

## Current ERC Summary
- Total messages: 48
- Errors: 1
- Warnings: 47
- Source: [Rev B/ERC_PREFAB_2026-06-29.rpt](Rev B/ERC_PREFAB_2026-06-29.rpt)
- Latest synced run (after alias/severity tuning): 35 messages, 1 error, 34 warnings.
- Source: [Rev B/ERC_SYNC_NOW_2026-06-29.rpt](Rev B/ERC_SYNC_NOW_2026-06-29.rpt)

## Change Coverage Audit (2026-06-29)
Original mic-scope intent vs current schematic/netlist implementation.

Implemented:
- INMP441 symbol and footprint are present as U2.
- Set A I2S mapping is present:
  - GPIO32 -> MIC_I2S_BCLK
  - GPIO33 -> MIC_I2S_WS
  - GPIO35 -> MIC_I2S_SD
- L/R is tied to GND (left-channel select behavior).
- U2 VDD is tied to +5V_CTRL.

Open/mismatch items:
- GPIO33 still reports ERC pin_not_driven and is not closed yet.
- Net naming mismatch from earlier plan: MIC_VDD/MIC_GND/MIC_LR_SEL are not currently named nets in netlist.
- Earlier header-only checklist text in handoff is stale relative to current U2 module implementation.
- Explicit schematic note text for optional/DNI intent was not found in current schematic text export.

## Must Fix Before Pass
1. Input pin not driven on GPIO33 path
- Finding: U1 pin 7 (GPIO33) flagged as input not driven.
- Location in report: [Rev B/ERC_PREFAB_2026-06-29.rpt](Rev B/ERC_PREFAB_2026-06-29.rpt#L5)
- Likely cause: ESP32 symbol pin electrical type for GPIO33 is input while this net is used as I2S WS output.
- Planned action in schematic:
  - Option A: Override/adjust pin electrical type so WS source is considered driven.
  - Option B: Apply a targeted No ERC marker only if the connection is verified intentional.
- Status: Open

## Microphone-Specific Schematic Checks
1. INMP441 signal mapping
- MIC_I2S_BCLK -> U1 GPIO32 -> U2 SCLK
- MIC_I2S_WS -> U1 GPIO33 -> U2 WS
- MIC_I2S_SD -> U1 GPIO35 -> U2 SD
- L/R -> GND
- Status: Verify in schematic and netlist before closing.

2. Microphone supply net
- Current netlist shows U2 VDD on +5V_CTRL.
- Confirm module is 5V-safe for both supply and logic interaction with ESP32.
- If not 5V-safe, move to +3.3V and regenerate ERC/netlist.
- Status: Open decision

## Warnings Triage (Can Be Accepted With Rationale)
1. Missing external symbol/footprint libraries
- Multiple warnings indicate local library paths unavailable in current CLI environment.
- Action: Verify symbols and footprints resolve correctly on your KiCad workstation.
- Acceptable if resolved in your active KiCad setup.

2. Unspecified-to-passive / unspecified-to-power-input pin type warnings
- Mostly from imported symbols using generic or unspecified pin types.
- Action: Review only changed paths first (mic path and nearby affected nets).
- Acceptable if electrical intent is clear and no real connectivity fault exists.

3. Off-grid endpoint warnings
- Present around existing circuitry.
- Action: Snap relevant wire endpoints to grid where practical.
- Acceptable if connectivity is confirmed and no accidental opens.

## Schematic Sign-Off Checklist
- [ ] Fix or explicitly waive the single ERC error on GPIO33 path.
- [ ] Confirm microphone VDD net choice (+5V_CTRL vs +3.3V) against actual module capability.
- [ ] Confirm mic pin mapping against intended WLED configuration.
- [ ] Regenerate ERC and store report as a new revision file.
- [ ] Record final accepted warnings rationale in handoff note.

## Next Step After Schematic Pass
Proceed to PCB reroute, then run full-board final check (ERC + DRC + connectivity review).
