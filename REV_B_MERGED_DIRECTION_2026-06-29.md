# Rev B Merged Direction (2026-06-29)

Purpose: unify the upstream Rev B change tracker direction with local Rev B microphone and schematic closeout work.

## Source Inputs
- REV_B_CHANGE_TRACKER.md
- NEXT_CHAT_QUICKSTART.md
- Rev B/PREFAB_SCHEMATIC_CHECKLIST_2026-06-29.md (from WIP snapshot branch)
- Rev B/REV_B_HANDOFF.md (from WIP snapshot branch)

## What Is Now Synced
- Local main is fast-forwarded to origin/main at commit 529a03d.
- Local pre-sync work is preserved in branch wip/pre-sync-2026-06-29 at commit 80bd3fa.

## Integrated Work Plan (Single Direction)

### Phase 1: Schematic close gate (must complete before reroute)
1. Close the one real ERC blocker on GPIO33 (input-not-driven) using one of:
   - pin-type correction, or
   - targeted No ERC waiver with rationale.
2. Finalize microphone VDD decision:
   - keep +5V_CTRL only if module compatibility is confirmed, else move to 3.3V.
3. Reconcile mic naming/documentation mismatch:
   - either add MIC_VDD/MIC_GND/MIC_LR_SEL style nets, or
   - update docs to match actual implemented net naming.
4. Ensure optional or DNI intent is visible in schematic notes.
5. Regenerate Rev B ERC and mark schematic gate pass only when errors are zero or explicitly waived.

### Phase 2: PCB reroute and Rev B layout must-haves
Execute the current must items from the tracker in dependency order:
1. RB-001 switch pull-up network in schematic.
2. RB-002 switch pull network routing near MCU pins.
3. RB-007 onboard LED serial chain first-LED orientation correction.
4. RB-003 onboard status LED section relocation.
5. RB-005 R16 relocation near source GPIO path.
6. RB-004 AHT20 relocation while preserving validated I2C behavior.
7. RB-006 main terminal block move for silkscreen readability.
8. RB-009 WLED setup silkscreen labels.
9. RB-010 documentation updates aligned with new silkscreen/setup flow.

### Phase 3: Firmware and validation lock
1. Keep guarded flash identity checks unchanged.
2. Run verification sequence per board candidate:
   - guarded upload precheck pass,
   - i2cscan returns 0x38,
   - aht20 plausible values,
   - switches stable and repeatable,
   - onboard and external LED function pass,
   - WLED setup succeeds from labels and docs.
3. Keep RB-008 as a conditional decision unless routing or SI evidence requires change.

## Immediate Next Session Task List
1. Bring Rev B files from wip/pre-sync-2026-06-29 into tracked main workstream.
2. Fix GPIO33 ERC status and lock mic VDD decision.
3. Re-run Rev B ERC and DRC after schematic updates.
4. Update REV_B_CHANGE_TRACKER.md status fields for completed schematic gates.
5. Start reroute with RB-001 and RB-007 first (highest functional risk).

## Notes
- Current tracker in main governs prioritization and status.
- WIP Rev B mic files remain the detailed implementation reference until merged onto main.
