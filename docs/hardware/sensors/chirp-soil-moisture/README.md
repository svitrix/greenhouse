# Chirp! — Capacitive Soil Moisture Sensor (I²C)

Capacitive soil moisture sensor (corrosion-free, unlike resistive types).
Irrigation trigger. Can also read a built-in thermistor and light sensor in addition to moisture (capacitance).

| Parameter | Value |
|---|---|
| Power | 3.3–5 V (**in this project — 3.3 V** for level compatibility) |
| Interface | I²C, address **`0x20`** by default (**configurable in firmware**) |
| Built-in pull-ups | **None** — external 4.7 kΩ required |
| Measurement type | Capacitive (corrosion-free) |
| Extras | built-in thermistor + light sensor |
| Cable | ~1 m |
| Wire colours | Red=VCC, Black=GND, Blue=SDA, Yellow=SCL |

---

## Wiring

Shares the common I²C bus with [AM2315C](../am2315c/README.md).

| Wire (colour) | ESP32-C6 |
|---|---|
| Red — VCC | **3V3** |
| Black — GND | GND |
| Blue — SDA | `GPIO6` (+ shared 4.7 kΩ → 3V3) |
| Yellow — SCL | `GPIO7` (+ shared 4.7 kΩ → 3V3) |

Sensor-node wiring diagram: [`../../sensor-node-wiring.svg`](../../sensor-node-wiring.svg).

---

## Gotchas

- ⚠️ **Power at 3.3 V** for level compatibility with ESP32-C6 (max 3.3 V input).
- **No built-in pull-ups** — 4.7 kΩ on SDA/SCL to 3V3 are mandatory (same resistors as AM2315C on the shared bus).
- **Address is configurable in firmware** (`SET_ADDRESS`) — assign different addresses for multiple Chirp! sensors on one bus.
- **Calibration is mandatory** — the raw capacitance value depends on the individual unit. Without calibration, moisture percentages are meaningless.

---

## Calibration

> Canonical values and the distinction between "firmware default" and "measured" values —
> [canonical-values.md#calibration](../../reference/canonical-values.md#calibration).
> This section covers the procedure and details.

Linear interpolation between "dry" (in air) and "wet" (in a cup of water):

```
moisture_pct = (raw - raw_dry) / (raw_wet - raw_dry) * 100
```

**Important — two distinct values, do not confuse them:**

- **Firmware default `300 / 700`** (`AppConfig::kDefaultSoilCalibration`) — a
  generic fallback used by `SoilNormalizer` **only when NVS `soil_calib` is empty**.
  Intentionally NOT the values of a specific sensor unit — the default must not
  hard-code one unit's calibration.
- **Measured `249 / 489`** (see [#measured-units](#measured-units)) — actual
  measured values for the physical unit. Stored in NVS `soil_calib` on the
  coordinator (entered during provisioning / via REST) and **override** the default
  at runtime.

The sensor-node sends **raw capacitance** over Zigbee and does not store calibration —
all `raw → %` conversion is done by `SoilNormalizer` on the coordinator.

Procedure:

1. Dry the sensor, read `GET_CAPACITANCE` in air → `raw_dry`.
2. Submerge the active zone in water (not the electronics) → `raw_wet`.
3. Save both values to NVS `soil_calib` (provisioning form or REST) —
   do **not** edit the firmware default.

### Measured units {#measured-units}

| Unit | `raw_dry` | `raw_wet` | When / how |
|---|---|---|---|
| unit-A | 249 | 489 | bench bring-up 2026-05-22 @3V3: air 30 s / cup of water to the mark |

When replacing the sensor — recalibrate and update this table + NVS.

---

## Register map

Full map — in `archive/i2c-moisture-sensor-master/src/main.c:26-35`.

| Register | Command | R/W |
|---|---|---|
| `0x00` | `GET_CAPACITANCE` | r2 |
| `0x01` | `SET_ADDRESS` | w1 (double write on FW ≥ 0x26) |
| `0x02` | `GET_ADDRESS` | r1 |
| `0x03` | `MEASURE_LIGHT` | w0 |
| `0x04` | `GET_LIGHT` | r2 |
| `0x05` | `GET_TEMPERATURE` | r2 (signed) |
| `0x06` | `RESET` | w0 |
| `0x07` | `GET_VERSION` | r1 |
| `0x08` | `SLEEP` | w0 |
| `0x09` | `GET_BUSY` | r1 |

---

## Driver

Custom thin implementation (**not** the Apollon77 JS library). The `ChirpSoilSensor`
adapter in `infrastructure/drivers/` implements the `ISensorChannel` port from
`domain/ports/`; wired in the composition root (`main.cpp`). The constructor takes
`TwoWire&` and the address explicitly — `Wire.begin()` lives in the composition root,
not in the driver.

---

## Related documents

- **Driver design:** [`../../superpowers/specs/2026-05-15-chirp-soil-driver-design.md`](../../../superpowers/specs/2026-05-15-chirp-soil-driver-design.md)
- **Driver decision:** [`../../decisions/2026-05-15-chirp-soil-driver.md`](../../../decisions/2026-05-15-chirp-soil-driver.md)
- **Sensor pipeline:** [`../../decisions/2026-05-15-sensor-pipeline.md`](../../../decisions/2026-05-15-sensor-pipeline.md)
- Full BOM (#9): [`COMPONENTS.md`](../../../../COMPONENTS.md)
