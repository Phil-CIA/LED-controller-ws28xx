# Rev B Optional MIC_I2S Provisioning Spec

Date: 2026-06-29
Scope: Provision-only support for future WLED audio-reactive microphone input.

## Intent
- Add optional microphone connectivity in Rev B without requiring immediate microphone population.
- Keep primary LED controller behavior unchanged when microphone is not installed.
- Prefer I2S digital microphone compatibility for WLED.

## Locked Module and Connector
- Module lock: INMP441 I2S microphone module, 14 mm form factor.
- Implementation lock: custom device footprint already placed as U2.
- KiCad symbol/library: 1My_IC's:INMP441.
- KiCad footprint/library: 1My_Parts_ ICs:inmp441.
- Physical interface: dual 1x3 headers (2.54 mm), total 6 pins.

## Electrical Interface
Required nets for the optional interface:
- MIC_VDD
- MIC_GND
- MIC_I2S_BCLK
- MIC_I2S_WS
- MIC_I2S_SD
- MIC_LR_SEL

Current netlist implementation:
- U2 VDD is tied to +5V_CTRL.
- U2 L/R is tied to GND.

Optional protection/conditioning footprints (DNI by default):
- 3x series resistors (one per I2S signal, nominal 22R to 100R)
- 100 nF decoupling capacitor at MIC_3V3
- 10 uF local bulk capacitor at MIC_3V3

## Locked Pin Map (Classic ESP32)
Set A is now locked:
- GPIO32 -> MIC_I2S_BCLK
- GPIO33 -> MIC_I2S_WS
- GPIO35 -> MIC_I2S_SD
- MIC_LR_SEL -> GND (left-channel select for WLED compatibility)

## Locked Pin Functions (U2)
1. L/R -> MIC_LR_SEL (tied to GND)
2. WS -> MIC_I2S_WS
3. SCLK -> MIC_I2S_BCLK
4. SD -> MIC_I2S_SD
5. VDD -> MIC_VDD (currently +5V_CTRL)
6. GND -> MIC_GND

## KiCad Schematic Tasks
1. Keep U2 as the locked INMP441 device symbol and footprint pair.
2. Verify the U2 six pin functions match the locked mapping above.
3. Keep microphone path optional in documentation/BOM intent.
4. Keep design local to Rev B schematic only.

## KiCad PCB Tasks
1. Place/keep the U2 footprint in a low-noise region away from switching and high current LED paths.
2. Keep I2S traces short and direct; avoid crossing noisy return paths.
3. Add clear silkscreen orientation and signal labels.
4. Leave microphone and optional passives as DNI unless intentionally enabled.

## Verification
- ERC: no new unresolved errors caused by MIC_I2S nets.
- DRC: no new clearance/connectivity violations.
- Pin conflict check: selected I2S pins do not conflict with existing mapped functions.
- Pinout check: U2 pin-function mapping matches the locked U2 list.

## Future Bring-Up
When a microphone is populated:
1. Enable WLED AudioReactive.
2. Configure I2S mode.
3. Enter the final BCLK, WS, SD GPIO mapping.
4. Save config and perform a hardware reset.
