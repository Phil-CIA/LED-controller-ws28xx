# My LED Controller Handoff

## Current state
Standalone WS281x bring-up project for an ESP32 board on COM5.

Breadboard unit status:
- WLED is running and controllable over Wi-Fi.
- Breadboard is now considered a closed unit for this phase.

Repository status:
- Firmware workflow now supports separate target profiles (`breadboard`, `pcb`) with independent identity lock.
- Active target is controlled via `scripts/flash-targets.json` (`activeTarget`) or `-TargetName` on scripts.

WLED migration support has been added:
- guarded WLED flash script: `scripts/guarded-wled-flash.ps1`
- target-aware discovery script: `scripts/discover-wled.ps1`
- target management scripts:
  - `scripts/set-active-target.ps1`
  - `scripts/set-target-mac.ps1`
- VS Code tasks are now split per target (`Breadboard:*`, `PCB:*`)

WLED firmware has been flashed successfully:
- Binary: `WLED_16.0.0_ESP32.bin`
- Port: `COM5`
- Chip: `ESP32-D0WD-V3`
- Detected MAC: `14:33:5C:2F:27:C4`
- MAC pinning is now set in `scripts/flash-targets.json`

Important correction:
- Direct single-file `esptool` flash attempts with release `WLED_16.0.0_ESP32.bin` caused boot resets with `invalid header`.
- Known-good recovery path is `scripts/guarded-flash.ps1 -Action upload` (restores custom firmware and boot chain).
- Recommended WLED path for this board is the official installer: `https://install.wled.me` (ESP32 target), then run `scripts/discover-wled.ps1`.

## Known-good hardware assumptions
- Board: ESP32 family, current upload target is `esp32dev`
- Data pin: GPIO18
- LED count: 300
- Level shifter: 74H125
- Serial/upload port: COM5 (can differ per target profile later)

## Known-good software state
- PlatformIO environments:
  - `esp32_led_breadboard`
  - `esp32_led_pcb`
- Framework: Arduino
- Platform pinned to pioarduino stable release
- Guarded upload scripts check port + chip + pinned MAC for the selected target before flashing
- Firmware currently includes:
  - rainbow animation
  - flashy multi-phase demo routine (`demo`) for long-run strip testing
  - solid color modes
  - brightness command
  - current-limit command (`limit<ma>`) with hard cap at 15000 mA
  - raw GPIO18 `probe` command
  - startup RGBW diagnostic pulse sequence
  - steady-state default current limit of 12000 mA

## What was verified
- Build succeeded
- Guarded upload succeeded to COM5
- Chip on COM5 was confirmed as ESP32-D0WD-V3
- Hidden solder bridge on pins 1 and 2 had been the cause of the no-signal condition and was fixed
- The strip was observed working after that fix
- Current-limit behavior was verified over serial:
  - `limit15000` applies as expected
  - values above 15000 mA are clamped to 15000 mA
  - `limit12000` returns to the steady-state target
- Demo mode was flashed and started successfully (`demo` -> "Mode: demo")

## Files to open first
- `README.md`
- `platformio.ini`
- `src/main.cpp`
- `scripts/guarded-flash.ps1`

## Useful commands
- Set active target to breadboard: `powershell -ExecutionPolicy Bypass -File scripts/set-active-target.ps1 -TargetName breadboard`
- Set active target to PCB: `powershell -ExecutionPolicy Bypass -File scripts/set-active-target.ps1 -TargetName pcb`
- Pin PCB MAC before first guarded flash: `powershell -ExecutionPolicy Bypass -File scripts/set-target-mac.ps1 -TargetName pcb -ExpectedMac <MAC>`
- Build selected target: `powershell -ExecutionPolicy Bypass -File scripts/guarded-flash.ps1 -Action build`
- Upload selected target: `powershell -ExecutionPolicy Bypass -File scripts/guarded-flash.ps1 -Action upload`
- Flash WLED (recommended): use `https://install.wled.me` for the currently connected target
- Discover breadboard WLED: `powershell -ExecutionPolicy Bypass -File scripts/discover-wled.ps1 -TargetName breadboard -OpenUi`
- Discover PCB WLED: `powershell -ExecutionPolicy Bypass -File scripts/discover-wled.ps1 -TargetName pcb -OpenUi`
- Monitor: `C:/Users/user/.platformio/penv/Scripts/platformio.exe device monitor -b 115200 -p COM5`
- Start demo: send `demo` in serial monitor
- Return to steady current target: send `limit12000`
- Set temporary higher test limit: send `limit15000`
- Turn LEDs off: send `off`

## Notes for the next session
- Do not reuse breadboard MAC for PCB profile. Keep each target identity pinned separately.
- For PCB bring-up, set PCB target MAC first; guarded flash intentionally fails if `expectedMac` is empty.
- Keep this project isolated from the older WorkStation repo unless you intentionally need to cross-reference it.
- If the LEDs stop responding again, first check for hardware shorts, power, and ground before changing firmware.
- If you want to simplify the firmware later, the startup probe sequence can be removed after bring-up is complete.
- Current policy: run steady around 12 A, allow up to 15 A max during testing/effect changes.
- If upload fails with COM5 busy/access denied, close any active serial monitor terminal first.
- After flashing WLED, set LED config in WLED UI to GPIO18, 300 LEDs, GRB, and set max current to 12000 mA.

## Session closeout (2026-06-29)

This session finalized repository synchronization and Rev B tracking publication.

### What was completed
- Created and linked the living Rev B planning artifacts:
  - `REV_B_CHANGE_TRACKER.md`
  - `NEXT_CHAT_QUICKSTART.md`
- Reconciled local workspace content with GitHub `main` using additive merge-style commits.
- Published firmware/docs/scripts that existed locally but were not yet tracked on GitHub.
- Cleaned workspace clutter and updated ignore rules for generated files.

### Latest GitHub state
- Remote: `https://github.com/Phil-CIA/LED-controller-ws28xx`
- Branch: `main`
- Current head at closeout: `7832305`
- Recent relevant commits:
  - `7832305` Merge local firmware workspace into repo (scripts, src, docs)
  - `d608d14` Add living Rev B change tracker and link quickstart

### Files now available on GitHub for laptop sync
- `HANDOFF.md`
- `README.md`
- `platformio.ini`
- `src/main.cpp`
- `scripts/discover-wled.ps1`
- `scripts/flash-targets.json`
- `scripts/guarded-flash.ps1`
- `scripts/guarded-wled-flash.ps1`
- `scripts/set-active-target.ps1`
- `scripts/set-target-mac.ps1`
- `REV_B_CHANGE_TRACKER.md`
- `NEXT_CHAT_QUICKSTART.md`

### Laptop sync steps
1. Open terminal in the laptop clone of this repo.
2. Run `git checkout main`.
3. Run `git pull origin main`.
4. Verify with `git log --oneline -n 3` and confirm commit `7832305` is present.

### Ongoing process
- Keep `REV_B_CHANGE_TRACKER.md` as the authoritative living change register for Rev B.
- Update `HANDOFF.md` at each session close with what changed, what was validated, and the latest commit reference.

## Session closeout (2026-06-29, Rev B pre-route handoff)

This closeout completes repository sync and publishes the Rev B baseline artifacts needed before board routing.

### What was completed
- Created a safety snapshot branch before sync: `wip/pre-sync-2026-06-29` at `80bd3fa`.
- Fast-forward synced local `main` to remote `origin/main`.
- Imported Rev B design and pre-fab artifacts into `main` and pushed to GitHub.
- Added merged direction plan for pre-route execution in `REV_B_MERGED_DIRECTION_2026-06-29.md`.

### Latest GitHub state
- Branch: `main`
- Current head at closeout: `0191824`
- Closeout commit: `0191824` Add Rev B design baseline and prefab mic closeout artifacts

### Rev B pre-route gate (must finish before routing)
1. Resolve or explicitly waive the GPIO33 ERC input-not-driven finding.
2. Lock microphone VDD decision (+5V_CTRL only if confirmed safe; else move to +3.3V).
3. Reconcile mic net naming/docs mismatch and ensure optional or DNI intent is explicit.
4. Re-run Rev B ERC and DRC with reports saved as new revision artifacts.

### Start-next-session focus
- Execute the pre-route gate items above first.
- After gate pass, begin RB-001 and RB-007 implementation before broader reroute work.
