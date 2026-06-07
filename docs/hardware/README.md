# Hardware Documentation

Hardware documentation for the Greenhouse ESP32-C6 project. **One fact — one home.**

> ## ⭐ Looking for a specific value (pin / I²C address / calibration)?
> → **[reference/canonical-values.md](reference/canonical-values.md)** — the single
> authoritative source. Everything else (board pages, sensor pages, connection matrices,
> `COMPONENTS.md`, `CLAUDE.md`, firmware comments) **links here**, never repeats the number.
> If the code and the table disagree — trust the code, fix the table
> ([maintenance rule](reference/canonical-values.md#maintenance-rule)).
>
> ## Looking for "what connects to what"?
> → **[connections/](connections/)** — connection matrices: pin → net →
> component → inline part → header.

---

## Documentation map

```
docs/hardware/
├── reference/canonical-values.md   ⭐ single source of truth (pins, addresses, calibration)
├── connections/                    what connects to what (matrices)
│   ├── coordinator-matrix.md
│   └── sensor-node-matrix.md
├── boards/                         MCU dev boards (chip, pinout, gotchas)
│   ├── esp32-c6-devkitm-1/
│   └── esp32-c6-supermini/
├── sensors/                        I²C sensors
│   ├── am2315c/
│   └── chirp-soil-moisture/
├── actuators/                      actuators
│   ├── relay-pump/
│   ├── transistors-bc547-bc557/
│   └── float-switch/               ⚠ STATUS: not installed (firmware = fake)
├── power/                          power circuits
│   ├── coordinator-power.md        USB-PD 12V → pump + buck 5V
│   └── sensor-node-battery.md      18650 → protection → LTH7R
├── coordinator-assembly.md         step-by-step coordinator assembly
├── datasheets/                     BC547, BC557
├── photos/                         assembly photos
└── *.svg                           sensor-node wiring diagrams
```

## Boards (MCU dev boards)

| Board | Role | Documentation |
|---|---|---|
| **ESP32-C6-DevKitM-1** | Coordinator: Zigbee coordinator, pump, Wi-Fi→MQTT→HA | [`boards/esp32-c6-devkitm-1/`](boards/esp32-c6-devkitm-1/README.md) · [matrix](connections/coordinator-matrix.md) · [power](power/coordinator-power.md) |
| **ESP32-C6 SuperMini** | Sensor-node: battery-powered Zigbee end device | [`boards/esp32-c6-supermini/`](boards/esp32-c6-supermini/README.md) · [matrix](connections/sensor-node-matrix.md) · [battery](power/sensor-node-battery.md) |

## Sensors

| Sensor | Measures | I²C address | Documentation |
|---|---|---|---|
| **ASAIR AM2315C** | Air temperature + humidity | `0x38` (fixed) | [`sensors/am2315c/`](sensors/am2315c/README.md) |
| **Chirp! Capacitive Soil** | Soil moisture (+ thermistor, light) | `0x20` (configurable in firmware) | [`sensors/chirp-soil-moisture/`](sensors/chirp-soil-moisture/README.md) |

Both share one bus `GPIO6/7 @ 100 kHz`, powered at 3.3 V, with shared external
4.7 kΩ pull-ups → 3V3. Full table —
[canonical I²C](reference/canonical-values.md#i2c).

## Actuators

| Actuator | Control | Status | Documentation |
|---|---|---|---|
| **Relay + pump 12 V** | GPIO18 (CH1), active-high, 20 s watchdog | ✅ working | [`actuators/relay-pump/`](actuators/relay-pump/README.md) |
| **Float switch** | GPIO14 (reserved) | ⚠ **not installed** (firmware = fake) | [`actuators/float-switch/`](actuators/float-switch/README.md) |
| **BC547 / BC557** | optional level-shift for relay | as needed | [`actuators/transistors-bc547-bc557/`](actuators/transistors-bc547-bc557/README.md) |

## Supporting materials

- **BOM (21 items):** [`COMPONENTS.md`](../../COMPONENTS.md) — purchase list and
  rationale. Pins/addresses/calibration are **not** duplicated there — linked here instead.
- **Coordinator assembly:** [`coordinator-assembly.md`](coordinator-assembly.md)
- **Diagrams:** [`sensor-node-wiring.svg`](sensor-node-wiring.svg) · [`sensor-node-breadboard.svg`](sensor-node-breadboard.svg)
- **Photos:** [`photos/`](photos/) · **Datasheets:** [`datasheets/`](datasheets/)
- **Driver decisions:** [`../decisions/`](../decisions/) · [`../superpowers/specs/`](../superpowers/specs/)

---

## How to maintain (anti-hallucination rule)

1. A hardware value changes → update the **firmware constant** (runtime truth), then
   mirror the row in [canonical-values.md](reference/canonical-values.md).
2. Other files link to the canonical table — no need to touch them unless the
   *structure* changed.
3. **Never** restate a value from the canonical table as an authoritative number —
   use an anchor link instead.
4. Before committing: `bash scripts/check-hw-consistency.sh`.
