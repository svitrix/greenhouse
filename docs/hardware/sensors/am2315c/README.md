# ASAIR AM2315C — Air Temperature / Humidity (I²C)

Digital temperature and humidity sensor in a sealed enclosure with a cable.
Monitors the greenhouse microclimate; reports `temperature` and `humidity` to Home Assistant.

| Parameter | Value |
|---|---|
| Power | 2.2–5.5 V (**in this project — strictly 3.3 V**, see gotchas) |
| Interface | I²C, address **`0x38`** (fixed) |
| Built-in pull-ups | **None** — external 4.7 kΩ required |
| Temperature | −40…+80 °C, ±0.3 °C, resolution 0.01 °C |
| Humidity | 0–100 %RH, ±2 %, resolution 0.024 % |
| Enclosure | IP-rated, 60 cm cable |
| Compatibility | functionally compatible with AHT20 |

---

## Wiring

Shares the common I²C bus with [Chirp!](../chirp-soil-moisture/README.md).

| Wire | ESP32-C6 |
|---|---|
| VCC | **3V3** |
| GND | GND |
| SDA | `GPIO6` (+ shared 4.7 kΩ → 3V3) |
| SCL | `GPIO7` (+ shared 4.7 kΩ → 3V3) |

Sensor-node wiring diagram: [`../../sensor-node-wiring.svg`](../../sensor-node-wiring.svg).

---

## Gotchas

- ⚠️ **Power strictly at 3.3 V, not 5 V.** The chip drives SDA/SCL at VCC level — 5 V would reach ESP32-C6 GPIO inputs (max 3.3 V) and destroy the pin.
- **No built-in pull-ups** — 4.7 kΩ on SDA and SCL to 3V3 are mandatory. The same resistors serve the shared bus with Chirp!.
- **Address `0x38` is fixed** — two AM2315C sensors cannot share one bus. For multiple air sensors use an I²C multiplexer or a second bus.

---

## Read protocol

I²C measurement command and response parsing:

1. Write measurement command `0xAC 0x33 0x00`.
2. Wait **~80 ms** (conversion time).
3. Read **6 bytes**: status + 20-bit RH + 20-bit T (+ CRC).

Driver — custom I²C implementation or Adafruit AHTX0 (AM2315C is compatible with
AHT20). The `AM2315CSensor` adapter lives in `infrastructure/drivers/` and implements
the `ISensorChannel` port from `domain/ports/`; wired in the composition root
(`main.cpp`). See layer rules in [`CLAUDE.md`](../../../../CLAUDE.md).

---

## Related documents

- Sensor pipeline: [`../../superpowers/specs/2026-05-15-sensor-pipeline-design.md`](../../../superpowers/specs/2026-05-15-sensor-pipeline-design.md)
- Pipeline decision: [`../../decisions/2026-05-15-sensor-pipeline.md`](../../../decisions/2026-05-15-sensor-pipeline.md)
- Full BOM (#4): [`COMPONENTS.md`](../../../../COMPONENTS.md)
