# ADR: Chirp! Soil Moisture Sensor Driver

> **Status:** Accepted — implemented in `firmware/sensor-node/lib/infrastructure/src/drivers/ChirpSoilSensor.hpp/.cpp`
> **Implements:** `ISensorChannel` (sensor-node plugin architecture)
> **I²C address:** 0x20 (configurable)

---

## D1 — Custom driver vs Apollon77 library

**Alternatives:**
- **A. Apollon77/I2CSoilMoistureSensor** — wrap in adapter.
  Holds `Wire` reference internally; depends on Arduino headers → breaks `native` env;
  constructor does not accept `TwoWire&` injection.
- **B. Custom thin driver (~120 lines).** Full control over quirks; clean DI
  (`TwoWire&` + address in ctor); compiles in `native` env via `#ifdef ARDUINO` guard.

**Choice: B.**

Why: the protocol is 10 commands and the reference firmware lives in
`archive/i2c-moisture-sensor-master`. Constitution I requires no Arduino headers in
`domain/`; Apollon77 violates this. Custom driver is trivially covered by hwtest
(`coordinator-hwtest` env).

---

## D2 — Read semantics: always-fresh blocking

**Chip quirk:** a read returns the value from the *previous* measurement window and
starts a new one (~10 ms conversion time). Post-reset, the first read is garbage —
reference firmware calls `getCapacitance()` twice on cold boot.

**Alternatives:**
- **A. Always-fresh blocking.** `read()` = dummy-write → wait GET_BUSY=0 (max 50 ms)
  → read. Two I²C roundtrips, ~15–25 ms. Semantics: "give me a value right now."
- **B. Pre-trigger in `init()` + one read per call.** Faster (~3–5 ms), but creates a
  hidden dependency on call period: if period < 10 ms, returns stale or garbage.
- **C. Async state machine.** 0 blocking, but leaks state into application layer.

**Choice: A.**

Why: correctness > 15 ms on an I²C bus that is idle 99.9% of the time (sensor reads
at 5-min intervals). Pre-trigger semantics are a silent correctness trap for any
on-demand REST read.

**Stuck-BUSY recovery (added in sensor-node phase):** if `GET_BUSY` stays asserted
>50 ms, driver calls `bus_.end(); bus_.begin()` to reset the I²C controller. Prevents
indefinite block if sensor locks up after cold power-cycle.

---

## D3 — Driver scope

**Choice: RESET + GET_VERSION on init; read capacitance + temperature only.**

| Command | Included | Reason |
|---|---|---|
| GET_CAPACITANCE (0x00) | ✅ | Core measurement |
| GET_TEMPERATURE (0x05) | ✅ | Core measurement |
| RESET (0x06) | ✅ on init | Deterministic state after MCU reboot / OTA |
| GET_VERSION (0x07) | ✅ on init | Early detection of wrong address / broken wiring |
| GET_BUSY (0x09) | ✅ internally | Stuck-BUSY recovery |
| GET_LIGHT (0x04) | ❌ | Reference README: "sensor is pretty noisy" |
| SLEEP (0x08) | ❌ | Unclear wake-up mechanism; saves ~5 mA, not worth the risk |
| SET_ADDRESS (0x01) | ❌ | Only one Chirp in current build; add when needed |

---

## D4 — Calibration placement

Raw capacitance (0–1023 range from Chirp's 10-bit ADC) must be converted to
`moisture_pct`. Two options: driver or application layer.

**Choice: coordinator-side `SoilNormalizer` in `application/`.**

Why: driver is an I²C talker — SRP says it should not know what "percent moisture"
means. `SoilNormalizer` is unit-testable without `Wire.h`. Calibration (`raw_dry`,
`raw_wet`) lives in NVS via `ISoilCalibrationStore` and is updatable over REST without
power-cycling the sensor rail.

---

## D5 — Sensor power rail gating

The sensor-node uses a p-MOSFET (GPIO4 gate) to cut sensor power between sleep
cycles. This is handled by `GpioPowerRail`, not by the driver itself.

**Init sequence in `SensorCycle::runOnce()`:**
1. `rail.on()` → GPIO4 LOW → sensors powered
2. Wait `registry.maxWarmupMs()` (Chirp needs ~1000 ms post power-up)
3. `probe()` all channels (NACK = absent, other error = faulty)
4. `read()` all present channels
5. `rail.off()` + `gpio_hold_en()` before deep sleep

The 1000 ms warmup is set by `ChirpSoilSensor::warmupMs()` and is the bottleneck for
the entire wake cycle.

---

## Register map (from Chirp firmware `main.c:26-35`)

| Cmd  | Name             | Dir | Len |
|------|------------------|-----|-----|
| 0x00 | GET_CAPACITANCE  | r   | 2   |
| 0x01 | SET_ADDRESS      | w   | 1   |
| 0x02 | GET_ADDRESS      | r   | 1   |
| 0x03 | MEASURE_LIGHT    | w   | 0   |
| 0x04 | GET_LIGHT        | r   | 2   |
| 0x05 | GET_TEMPERATURE  | r   | 2   |
| 0x06 | RESET            | w   | 0   |
| 0x07 | GET_VERSION      | r   | 1   |
| 0x08 | SLEEP            | w   | 0   |
| 0x09 | GET_BUSY         | r   | 1   |

Temperature is signed int16, units 1/10°C (range −20.0…+85.0 °C).
`SET_ADDRESS` with FW ≥ 0x26 requires writing the new address twice.
