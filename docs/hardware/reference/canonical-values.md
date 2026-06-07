# Canonical Hardware Values — Single Source of Truth

> **This file is authoritative.** Every pin number, I²C address, bus frequency,
> timeout and calibration value in this project is defined **here, once**. Board
> pages, sensor pages, connection matrices, `COMPONENTS.md`, the `CLAUDE.md`
> files, and firmware comments all reference these anchors instead of restating
> the number.
>
> If a value here disagrees with code, **the code is the runtime truth** —
> update this table to match and fix whatever drifted (see
> [maintenance rule](#maintenance-rule)). Each row names the firmware constant so
> the two can be diffed mechanically.

Verified against firmware on **2026-06-05** (commit `a76ac33`).

---

## I²C bus (shared by both boards) {#i2c}

| Key | Value | Firmware constant (file) | Notes |
|---|---|---|---|
| `i2c.sda` | **GPIO6** | `kI2cSdaPin=6` — `AppConfig.hpp`, `SensorNodeConfig.hpp`; `Wire.begin(6,7,…)` in both `main.cpp` | `LP_I2C_SDA` — pollable from deep-sleep on sensor-node |
| `i2c.scl` | **GPIO7** | `kI2cSclPin=7` — same files | `LP_I2C_SCL` |
| `i2c.freq` | **100 kHz** | `kI2cFrequencyHz=100'000` | standard mode |
| `i2c.pullups` | external **4.7 kΩ → 3V3**, one pair shared by all bus devices | — (hardware) | neither sensor has internal pull-ups |

## I²C devices {#i2c-devices}

| Key | Value | Firmware constant | Notes |
|---|---|---|---|
| `chirp.addr` | **0x20** (default, SW-changeable) | `kChirpAddress=0x20` — both configs | [Chirp page](../sensors/chirp-soil-moisture/README.md) |
| `chirp.warmup` | **1000 ms** | `kWarmupMs=1000` — `ChirpSoilSensor.hpp` | power-on settle |
| `am2315c.addr` | **0x38** (fixed) | `kAm2315cAddress=0x38` — both configs | [AM2315C page](../sensors/am2315c/README.md) |

## Soil calibration (Chirp) {#calibration}

> ⚠ This is the project's most-confused value. There are **two distinct things**;
> keep them separate.

| Key | Value | Where it lives | Meaning |
|---|---|---|---|
| `calibration.firmware_default` | **raw_dry=300 / raw_wet=700** | `AppConfig::kDefaultSoilCalibration` (`AppConfig.hpp`) → seeds `SoilNormalizer` | **Generic fallback only**, used when NVS `soil_calib` is empty. Intentionally *not* one specific sensor's numbers. |
| `calibration.measured.unit-A` | **raw_dry=249 / raw_wet=489** | NVS `soil_calib` on the coordinator (set via provisioning/REST); recorded in [Chirp page](../sensors/chirp-soil-moisture/README.md#measured-units) | The **measured** calibration of the physical Chirp unit bench-tested 2026-05-22 @3V3. Overrides the default at runtime. |

**Rules:**
- The sensor-node ships **raw capacitance** over Zigbee; it stores **no** calibration. `SoilNormalizer` on the coordinator converts raw → %.
- A per-unit calibration in NVS **overrides** the firmware default. Calibration is therefore *per physical sensor* and must be (re)entered when a sensor is swapped.
- The firmware default stays `300/700` on purpose — a shipped default must not encode one unit's bench numbers. To use unit-A out of the box, store `249/489` in NVS at provisioning, do **not** edit the default.

## Coordinator (ESP32-C6-DevKitM-1) GPIO {#coordinator-gpio}

| Key | Value | Header | Firmware constant | Notes |
|---|---|---|---|---|
| `coord.pump_relay` | **GPIO18** | J3.9 | `kPumpRelayGpio=18` (`AppConfig.hpp`), `kRelayIn1Pin=18` (`CoordinatorConfig.hpp`) | relay CH1 → pump; active-high |
| `coord.pump_active_high` | **true** | — | `kPumpRelayActiveHigh=true` | relay jumper set HIGH-active |
| `coord.relay_ch2` | **GPIO19** | J3.8 | — | spare relay channel, reserved |
| `coord.float_switch` | **GPIO14** (reserved) | J1.12 | — (no constant) | active-low `INPUT_PULLUP`. **Hardware NOT installed** — firmware uses `FakeFloatSwitchAlwaysOk`. See [float-switch page](../actuators/float-switch/README.md). |
| `coord.status_led` | **GPIO8** | J1.9 | — | onboard WS2812; strapping pin — do not repurpose |
| `coord.boot_button` | **GPIO9** | J3.11 | `kBootButtonGpio=9` | BOOT + strapping; provisioning trigger; no external load |
| `pump.max_runtime` | **20 s** | — | `kPumpMaxRuntimeMs=20'000` | safety cutoff — never disable |

## Sensor-node (ESP32-C6 SuperMini) GPIO {#sensor-node-gpio}

| Key | Value | Firmware constant | Notes |
|---|---|---|---|
| `node.sensor_power_gate` | **GPIO4** | `kSensorPowerGateGpio=4` | p-MOSFET gate; HIGH = sensors OFF. **Requires external 100 kΩ pull-up → VBAT** (strapping pin). |
| `node.battery_adc` | **GPIO0** (ADC1_CH0) | `kBatteryAdcGpio=0` | divider midpoint |
| `node.battery_divider` | **R1=100 kΩ, R2=100 kΩ** (Vbat→R1→ADC→R2→GND) | `kBatteryDividerR1Ohm`, `kBatteryDividerR2Ohm` | ÷2 divider |
| `node.status_led` | **GPIO8** | — | onboard WS2812; strapping pin |
| `node.user_led` | **GPIO15** | — | onboard user LED; strapping pin — do not drive |
| `node.usb` | **GPIO12 (D−) / GPIO13 (D+)** | — | native USB; only flash/log path — never repurpose |
| `node.uart0` | **GPIO16 (TX) / GPIO17 (RX)** | — | **not exposed** on SuperMini headers (no USB-UART bridge) |

## Chip (ESP32-C6FH4 — both boards) {#chip}

| Key | Value | Notes |
|---|---|---|
| `chip.core` | RISC-V, 1 HP @ 160 MHz + 1 LP | single HP core |
| `chip.ram` | 512 KB SRAM | |
| `chip.flash` | 4 MB (in-package) | fixes `partitions.csv` ceiling |
| `chip.io_voltage` | **3.3 V** (max 3.3 V in) | 5 V on any GPIO destroys the pin |
| `chip.io_current` | 40 mA source / 28 mA sink per pin | |
| `chip.adc` | ADC1 only, `GPIO0..GPIO6` (no ADC2) | `analogRead()` only on these |
| `chip.lp_gpio` | `GPIO0..GPIO7` | deep-sleep pollable |
| `chip.unexposed` | `GPIO10`, `GPIO11` | internal SPI-flash, not on either board |
| `chip.strapping` | `GPIO4, 5, 8, 9, 15` | high-Z until reset completes |

## Key timeouts / periods {#timing}

| Key | Value | Firmware constant | |
|---|---|---|---|
| `coord.sensor_period` | 10 s | `kSensorPeriodMs=10'000` | coordinator local tick (legacy) |
| `coord.irrigation_tick` | 1 s | `kIrrigationTickMs=1'000` | safety state-machine cadence |
| `node.report_period.default` | 60 s | `kReportPeriodS` / `kDefaultReportPeriodS` | configurable via ZCL 0xFF00 |
| `node.report_period.range` | 60 … 3600 s | `kReportPeriodMinS/MaxS` | clamped both ends |
| `node.steering_timeout` | 60 s | `kZbSteeringTimeoutMs=60'000` | first-join window |
| `node.failed_join_sleep` | 1 h | `kFailedJoinSleepMs` | economical retry |

---

## Maintenance rule {#maintenance-rule}

When a hardware fact changes:

1. **Update the firmware constant** (the runtime truth) — `AppConfig.hpp`,
   `CoordinatorConfig.hpp`, `SensorNodeConfig.hpp`, or `Wire.begin(...)`.
2. **Update the matching row here** so this table mirrors the code.
3. Everything else (board/sensor pages, matrices, `COMPONENTS.md`, `CLAUDE.md`)
   links to this table and needs no edit unless the *structure* changed.
4. Run the consistency check before committing:
   `bash scripts/check-hw-consistency.sh` (greps these rows vs the config headers).

Do **not** restate a value from this table anywhere else as an authoritative
number. Link to the anchor (e.g. `[GPIO18](../reference/canonical-values.md#coordinator-gpio)`)
instead. One fact, one home.
