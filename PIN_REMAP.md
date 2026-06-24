# ESP32 Classic Pin Remap for WLED (LED-controller-ws28xx)

## WLED Data Channel Mapping (current schematic nets)

| WLED Channel | ESP32 GPIO | 74AHCT125 Path | Board Net | Board Connector Target |
|---|---:|---|---|---|
| Main Strip Data | GPIO18 | U4 2A -> 2Y | `LED_STRIP` | `U9` LED_STRIP_CONN pin 2 (data) |
| Spare Output 1 | GPIO16 | U4 3A -> 3Y | `3Y` | `J4` SPARE_OUT_HDR pin 1 |
| Spare Output 2 | GPIO17 | U4 4A -> 4Y | `4Y` | `J4` SPARE_OUT_HDR pin 2 |
| Status LED Driver | GPIO19 | U4 1A -> 1Y | `STATUS_LED` | Status LED path (`R16`/`D1`) |

## New Spare Header (SMT)

| Header | Pin | Net | Purpose |
|---|---:|---|---|
| `J4` SPARE_OUT_HDR | 1 | `3Y` | Spare buffered data output #1 |
| `J4` SPARE_OUT_HDR | 2 | `4Y` | Spare buffered data output #2 |
| `J4` SPARE_OUT_HDR | 3 | `GND` | Signal reference |
| `J4` SPARE_OUT_HDR | 4 | `+5V_CTRL` | Optional pull-up/peripheral power |

## ESP32 Classic GPIO Capability Flags

### Input-only GPIOs (do not use for LED data outputs)
`GPIO34`, `GPIO35`, `GPIO36`, `GPIO39`

### Flash-reserved GPIOs (do not use)
`GPIO6`–`GPIO11`

### Boot-strapping sensitive GPIOs (usable with caution)
`GPIO0`, `GPIO2`, `GPIO15`

### Actively used in this design
`GPIO12`, `GPIO13`, `GPIO14`, `GPIO16`, `GPIO17`, `GPIO18`, `GPIO19`, `GPIO23`, `GPIO34`, `GPIO35`, `GPIO36`, `GPIO39`

## WLED Recommendation
Use `GPIO18` as the primary strip output (`LED_STRIP`). Keep `GPIO16` and `GPIO17` as optional secondary WLED outputs via `J4` when multi-channel outputs are needed. Avoid assigning WLED data outputs to GPIO34/35/36/39 because they are input-only on classic ESP32.
