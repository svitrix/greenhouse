# Connection Matrix — Sensor-node (ESP32-C6 SuperMini)

> **What connects to what** on a battery sensor-node, in one place. Pin numbers
> mirror [canonical-values.md](../reference/canonical-values.md#sensor-node-gpio)
> (authoritative); this page shows the *wiring*.
>
> Board layout & gotchas: [SuperMini board page](../boards/esp32-c6-supermini/README.md).
> Battery chain: [sensor-node power](../power/sensor-node-battery.md).
> Visual: [sensor-node-wiring.svg](../sensor-node-wiring.svg) ·
> [sensor-node-breadboard.svg](../sensor-node-breadboard.svg).

## Power nets

| Net | Source | Feeds | Notes |
|---|---|---|---|
| **VBAT** | 18650 (VTC6) → 1S protection → LTH7R charger (expansion) | board 5V/VBAT pad; MOSFET source; GPIO4 pull-up | ~3.0–4.2 V |
| **3V3 (gated)** | onboard LDO, switched by the p-MOSFET | AM2315C VCC, Chirp VCC, I²C pull-ups | OFF in deep sleep to save power |
| **GND** | common | everything | |

## Signal matrix (GPIO → net → endpoint)

| GPIO | Net / signal | Connects to | Inline part | Direction |
|---|---|---|---|---|
| **GPIO4** | sensor-power gate | p-MOSFET gate | **external 100 kΩ pull-up → VBAT** (mandatory; strapping pin) | out — HIGH = sensors OFF |
| **GPIO6** | I²C SDA | AM2315C + Chirp | 4.7 kΩ pull-up → gated 3V3 | bidir |
| **GPIO7** | I²C SCL | AM2315C + Chirp | 4.7 kΩ pull-up → gated 3V3 | bidir |
| **GPIO0** | battery sense | divider midpoint | 100 kΩ (R1) to VBAT, 100 kΩ (R2) to GND | in, ADC1_CH0 |
| **GPIO8** | WS2812 status LED | onboard LED | — | out, board-driven |
| **GPIO15** | user LED | onboard LED | — | strapping — do not drive |
| **GPIO12/13** | native USB D−/D+ | USB-C | — | flash/log only — never repurpose |

### Sensor power-gate wiring

```
VBAT ──┬── 100 kΩ ──┬── GPIO4 (gate)        (pull-up: gate HIGH during boot high-Z → MOSFET OFF)
       │            │
       └── MOSFET source         MOSFET gate ── GPIO4
            MOSFET drain ── sensor 3V3 rail ──┬── AM2315C VCC
                                              ├── Chirp VCC
                                              └── 4.7 kΩ ×2 ── SDA/SCL pull-ups
```

GPIO4 drives the p-MOSFET that powers the whole sensor rail. **HIGH = MOSFET off =
sensors off** (deep-sleep default). The external 100 kΩ pull-up to VBAT keeps the
gate high during the high-Z boot window (GPIO4 is a strapping pin); `gpio_hold_en`
latches it across deep sleep. Without the pull-up the MOSFET briefly conducts on
every boot. Details: [SuperMini board page](../boards/esp32-c6-supermini/README.md).

### I²C bus

Both sensors share one bus (GPIO6/7) and one pair of 4.7 kΩ pull-ups to the
**gated** 3V3 — so the pull-ups also de-power in deep sleep. Addresses:
Chirp `0x20`, AM2315C `0x38`
([canonical](../reference/canonical-values.md#i2c-devices)).

## Cross-check

Every pin above must equal its
[canonical row](../reference/canonical-values.md#sensor-node-gpio) and the
`SensorNodeConfig.hpp` constant named there.
