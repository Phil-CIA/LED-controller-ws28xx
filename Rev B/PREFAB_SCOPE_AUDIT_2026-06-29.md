# Rev B Pre-Fab Scope Audit (Broadened)

Date: 2026-06-29

## Why this audit
The scope was broadened from mic-only checks to full repo-level pre-fab context, including library environment consistency and DRC/ERC interpretation.

## Evidence reviewed
- Root ERC baseline: [ERC_ROOT_BASELINE_2026-06-29.rpt](../ERC_ROOT_BASELINE_2026-06-29.rpt)
- Rev B ERC baseline: [ERC_PREFAB_2026-06-29.rpt](ERC_PREFAB_2026-06-29.rpt)
- Rev B ERC after local lib-table sync: [ERC_PREFAB_2026-06-29_LIBTABLE_SYNC.rpt](ERC_PREFAB_2026-06-29_LIBTABLE_SYNC.rpt)
- Rev B DRC baseline: [DRC_PREFAB_2026-06-29.rpt](DRC_PREFAB_2026-06-29.rpt)
- Rev B netlist: [My LED controller Rev B.net](My%20LED%20controller%20Rev%20B.net)

## Key conclusion
Library warnings are not primarily caused by copying to Rev B. Similar library-name issues exist at root level. Rev B copy did not introduce the class of warnings; it exposed existing library-name/environment dependencies.

## Issue classes and status

1. Electrical pre-route blockers (must close)
- Rev B ERC still has one real error: GPIO33 pin_not_driven on MIC_I2S_WS path.
- Status: Open.

2. Microphone power assumption (must decide)
- Netlist currently shows U2 VDD tied to +5V_CTRL.
- This is acceptable only if the selected 14 mm module is explicitly 5V-safe.
- Status: Open decision.

3. Library-name mismatches (environment/tooling)
- Missing symbol-library names in ERC include: Espressif, easyeda2kicad, 1My Basic Componets, 1My Passive.
- Missing footprint-library names in ERC include: PCM_Espressif, PCM_4ms_Package_SSOP.
- Status: Open, non-routing-critical if local overrides are accepted and symbol/footprint resolution works in your KiCad UI.

4. DRC unconnected items (routing stage artifacts)
- Rev B DRC reports 121 unconnected items and 17 violations.
- Large unconnected count is expected pre-reroute and not a library-copy symptom.
- Status: Expected at this stage; to be re-evaluated after reroute.

## Actions completed in this pass
- Copied global KiCad tables into Rev B project folder:
  - [sym-lib-table](sym-lib-table)
  - [fp-lib-table](fp-lib-table)
- Re-ran ERC after sync. Result stayed materially the same, confirming warnings are primarily library-name mapping/alias issues, not absence of table files alone.

## Recommended closeout order
1. Clear/waive GPIO33 pin_not_driven intentionally.
2. Confirm INMP441 module supply requirement and lock VDD net (+5V_CTRL or +3.3V).
3. Perform PCB reroute.
4. Re-run ERC + DRC and then triage remaining library-name warnings separately from electrical/layout errors.

## Notes on library portability
Current library resolution depends on absolute OneDrive/AppData paths. For portable repo builds, migrate required custom symbols/footprints into a repo-scoped library set and update project tables to relative paths.
