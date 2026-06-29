# Rev B Change Tracker (Living Document)

## Intent
This is the working change-control document for the next PCB revision.

Scope:
- Hardware schematic changes
- PCB layout changes
- Firmware changes
- Manufacturing and assembly changes
- Test and validation changes
- Documentation updates

Policy:
- This document is intentionally changeable at any time.
- New items can be added, edited, re-prioritized, or deferred as findings evolve.
- Nothing is considered locked until the Rev B freeze decision is recorded in the Decision Log.

## Baseline Constraints To Preserve
- Keep strict guarded flash identity checks (port, chip, MAC).
- Preserve known-good behavior unless a change is explicitly approved.
- Avoid COM port shopping and stop on target mismatch.

## Current Known-Good Validation Baseline
- External strip path validated on GPIO18.
- Onboard 3-LED path validated on GPIO19.
- AHT20 validated on I2C SDA GPIO21 and SCL GPIO22 at address 0x38.
- Guarded upload working for PCB target on COM6.

## Active Issue and Change Register

Status values: Open, In Progress, Blocked, Done, Deferred.

| ID | Category | Change Request | Priority | Status | Dependency | Validation Gate | Risk If Deferred |
|---|---|---|---|---|---|---|---|
| RB-001 | Hardware schematic | Add external pull-up network for switch lines GPIO34/35/36/39 (active-low switches) | Must | Open | None | `switches` command stable and repeatable | Unreliable switch input behavior |
| RB-002 | PCB layout | Route and place switch pull network near MCU pins | Must | Open | RB-001 | Switch line noise and false-trigger checks pass | Continued switch instability |
| RB-003 | PCB layout | Relocate onboard status LED section | Must | Open | RB-007 | Onboard LEDs pass startup and command tests | Congested routing and placement conflicts |
| RB-004 | PCB layout | Relocate AHT20 circuit to free spare header strip placement area | Must | Open | Schematic net keep or approved remap | `i2cscan` finds 0x38, `aht20` plausible readings | Header placement blocked or sensor regression |
| RB-005 | Hardware + layout | Relocate R16 (series resistor on onboard LED data path) near source GPIO path | Must | Open | RB-003 | Onboard data signal integrity verified | Data edge quality and debugging difficulty |
| RB-006 | PCB layout | Move main terminal block for silkscreen readability | Must | Open | None | Labels readable and polarity unmistakable | Wiring/setup errors in assembly and field |
| RB-007 | Hardware + layout | Correct onboard LED serial chain first-LED orientation (DIN path) | Must | Open | None | Onboard chain responds correctly in order | Onboard LED chain malfunction |
| RB-008 | Hardware decision | Dedicated level shift for onboard LEDs | Nice | Open | RB-003 routing review | SI review outcome documented | Possible margin loss in noisy scenarios |
| RB-009 | WLED optimization | Add clear silkscreen labels for WLED setup points (GPIO18 data, GND, 5V, current target note) | Must | Open | RB-006 | Setup can be completed from silkscreen + docs | Higher setup error rate |
| RB-010 | Documentation | Add concise WLED setup instructions to docs and board labels | Must | Open | RB-009 | New user can configure WLED without guesswork | Slower bring-up and support overhead |
| RB-011 | Pre-route schematic gate | Resolve or explicitly waive GPIO33 ERC input-not-driven finding on Rev B mic path | Must | Open | None | Rev B ERC shows 0 errors or documented waiver accepted | Routing starts with unresolved electrical intent |
| RB-012 | Pre-route hardware decision | Lock Rev B microphone VDD rail decision (+5V_CTRL or +3.3V) and apply schematic update if required | Must | Open | RB-011 | Schematic and netlist match chosen rail and rationale recorded | Risk of mic supply incompatibility or rework |
| RB-013 | Pre-route documentation consistency | Reconcile mic net naming and optional or DNI documentation with implemented U2 module wiring | Must | Open | RB-012 | Handoff, checklist, and schematic notes match implementation | Build ambiguity and bring-up confusion |

## WLED-First Board Guidance (Working)
- Keep external strip data path aligned with current known-good GPIO18 unless routing forces change.
- Keep onboard LED path simple by default (no dedicated level shifter unless SI evidence requires it).
- Keep WLED setup discoverable from silkscreen and docs.

## Verification Checklist (Per Build Candidate)
- Guarded upload precheck passes for PCB target.
- `i2cscan` detects 0x38.
- `aht20` returns plausible temperature/humidity.
- `switches` reports stable button states.
- External strip and onboard LEDs both pass functional tests.
- WLED setup succeeds using documented/silkscreened instructions.

## Decision Log
Use this section to record decisions that freeze scope.

| Date | ID | Decision | Rationale | Approved By |
|---|---|---|---|---|
| 2026-06-24 | RB-008 | Do not add dedicated onboard LED level shifter by default | Keep complexity and BOM lower unless routing/SI review shows need | Working decision |

## Change Log
| Date | Summary |
|---|---|
| 2026-06-24 | Initialized living Rev B tracker with known baseline and newly requested changes. |
| 2026-06-29 | Added explicit pre-route gate items RB-011 through RB-013 for Rev B mic/ERC closeout before routing. |

## Update Rules
When adding or changing an item:
1. Add or update the ID in Active Issue and Change Register.
2. Set Priority and Status.
3. Add clear Validation Gate text.
4. If scope is frozen, record in Decision Log.
5. Add one line to Change Log.
