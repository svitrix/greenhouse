# CLAUDE.md — `firmware/sensor-node/`

> This file is a local guide for AI / engineers working on the sensor-node firmware. Chip-level facts (ESP32-C6 capabilities, strapping pins, ADC1 constraints, "which pin should I use" decision tree) — `../CLAUDE.md` (`firmware/CLAUDE.md`). Clean architecture, code conventions, error handling, security — root `/Users/bladerunner/Work/personal_projects/green-house-esp32/CLAUDE.md`. Hard embedded rules — `~/Work/CLAUDE.md`. Full architectural design — `docs/superpowers/specs/2026-05-17-split-coordinator-sensor-node-design.md`.

---

## [0] What this even is

The sensor-node is a **battery-powered wireless node** on the ESP32-C6 SuperMini. Its single job: wake up periodically, read sensors (AM2315C air temp/humidity + Chirp soil moisture), send the values to the coordinator over **Zigbee**, and go back to deep sleep. The coordinator runs on mains power — it receives this data, caches it, publishes to Home Assistant via MQTT, and decides "time to water or not".

**Target hardware:**
- **Board:** ESP32-C6 SuperMini (generic XIAO-format clone — no official vendor, see [`../../COMPONENTS.md`](../../COMPONENTS.md) #16). Same ESP32-C6FH4 chip / ESP32-C6-MINI-1 module as DevKitM-1. Board-level differences from DevKitM-1 — §0.1 below.
- **Power:** 3.7 V LiPo 1000–2000 mAh through a TP4056 charge module (USB-C → LiPo). The on-board TP4054 on the SuperMini is **not** used in the current assembly (would parallel two linear chargers — see `COMPONENTS.md` #17).
- **Sensors:** AM2315C @ 0x38 + Chirp @ 0x20 on the same I²C bus (GPIO6/7), both powered through a p-MOSFET gate driven by GPIO4.
- **Connectivity:** IEEE 802.15.4 / **Zigbee** End Device only (sleepy, `rx_on_when_idle = false`). **Wi-Fi is never initialised** — this is a battery device.
- **Storage:** NVS only (`Preferences`) for Zigbee pairing state (namespace `zigbee_pair`). No LittleFS, no calibration storage — calibration lives on the coordinator (see §3.6).
- **Partitions:** [`partitions.csv`](partitions.csv) — single-app 4 MB layout. OTA is not implemented (USB-C re-flash only); deferred to a possible Phase D.

**Target pinout** (matched against this specific SuperMini clone on bench bring-up; see [`../../COMPONENTS.md`](../../COMPONENTS.md) #16 for the rationale on using the DevKitM-1 board definition):

| GPIO  | Function                            | Note                           |
|-------|-------------------------------------|--------------------------------|
| 0     | ADC1_CH0 — battery voltage divider  | 2× 100 kΩ: Vbat → R1 → ADC → R2 → GND |
| 4     | p-MOSFET gate (sensor power)        | drive HIGH = MOSFET off = sensors off. **HARDWARE REQUIREMENT: external 100 kΩ pull-up to VBAT on the gate line.** GPIO4 is a strapping pin; without the pull-up the gate floats during the brief high-Z boot window and the MOSFET briefly conducts. `gpio_hold_en` is asserted in `GpioPowerRail::off()` to latch HIGH across deep sleep. |
| 6     | I²C SDA (LP_I2C_SDA-capable)        | 4.7 kΩ pull-up to gated 3V3    |
| 7     | I²C SCL                             | 4.7 kΩ pull-up                 |
| 8     | Onboard WS2812 status LED           | strapping pin — boot-safe as output |
| 12/13 | native USB D− / D+                  | **DO NOT USE** as a load       |

---

## [0.1] SuperMini board-level details

> Chip-level facts (ESP32-C6 subsystems, ADC1, LP, strapping, "which pin to use") are in [`../CLAUDE.md`](../CLAUDE.md) — they apply equally to SuperMini and DevKitM-1. This section is **SuperMini only**, focusing on what differs from DevKitM-1.

### What is on the board

| Component | Note |
|---|---|
| Form factor | ~26×18 mm, XIAO-style (3.3 V / GND / pin headers on two short edges + B+/B− pads on the back) |
| 5V → 3.3V LDO | powered from USB-C or from VBAT through TP4054 |
| Native USB-C | the **only** USB connector — there is **no separate USB-UART bridge** (unlike DevKitM-1) → `Serial` logs go through USB-CDC, not GPIO16/17 |
| Boot Button | on GPIO9 (same strapping pin as DevKitM-1). Boot + Reset → Firmware Download mode |
| Reset Button | system reset |
| RGB LED (WS2812) | on GPIO8 (same as DevKitM-1) |
| Separate user LED | on GPIO15 (DevKitM-1 has no such LED) — **strapping pin**, do not drive externally |
| TP4054 LiPo charger | on-board, 100 mA charge current, B+/B− pads on the back. **Not used in our assembly** — charging goes through LTH7R on the expansion board (`COMPONENTS.md` #17) to avoid two parallel linear chargers. |
| GPIO16/17 headers | **NOT exposed** on the SuperMini pin headers (unlike DevKitM-1 where they are on J3.2/J3.3 as `TX/RX`). HP UART0 still exists internally but is unreachable on this board. |

### Board-specific hard constraints (where SuperMini differs from DevKitM-1)

1. **No USB-UART bridge.** `Serial.print` only works after enabling USB-CDC: `-DARDUINO_USB_CDC_ON_BOOT=1 -DARDUINO_USB_MODE=1` in `platformio.ini` (or via `Serial.begin()` with native USB enumeration). **The current `platformio.ini` does NOT enable these flags** (tracked as open question Q1 in §5) — Serial logs require an external UART pad until that changes.
2. **GPIO12/13 are extra-strict.** They are the only way to flash / talk to the chip (no fallback through a UART bridge) — never repurpose them for sensors, even temporarily, or you brick remote flashing.
3. **GPIO15 is occupied by an on-board user LED.** Not a problem for our wiring (we don't touch GPIO15), but be aware: it is **both** a strapping pin **and** a board-driven LED, so external load on it is doubly risky.
4. **`board = esp32-c6-devkitm-1` in `platformio.ini` is intentional.** pioarduino does not ship a SuperMini board definition; the DevKitM-1 definition picks the correct MCU/flash params. The only practical difference at build time is the USB-CDC flags above. Documented in [`../../COMPONENTS.md`](../../COMPONENTS.md) #16.
5. **GPIO10 / GPIO11 not exposed** — same module (MINI-1) as DevKitM-1, same constraint.

### Spare pins (room for expansion)

- ADC-capable: `GPIO0..GPIO5` (CH0..CH5) — GPIO0 is occupied by the battery divider, GPIO6 by I²C SDA. So `GPIO1..GPIO5` are spare for ADC.
- Digital-only: `GPIO18..GPIO23` on this specific clone — verify the exposed subset on the bench before wiring any new load.
- LP-capable: `GPIO1..GPIO3, GPIO5` (GPIO0/4/6/7 already used; LP_GPIO8+ does not exist).

---

## [1] Current state — Phase B complete + plugin refactor

The sensor-node is a **working Zigbee End Device**. Phase A (stub) and Phase B (real Zigbee + drivers + deep sleep) are both done, plus the Phase 1 plugin-architecture refactor on top.

**What runs on the board now:**
- `src/main.cpp` (~95 lines): `setup()` brings up `SerialLogger`, kills Wi-Fi/BT, initialises `GpioPowerRail` (GPIO4 MOSFET gate), Wire bus, three `ISensorChannel` adapters (`AM2315CSensor`, `ChirpSoilSensor`, `BatteryMonitor`), wires them into a `SensorRegistry`, runs `registry.probeAll(rail, log)`, then `ZigbeeEndDeviceAdapter::start()`, then exactly one `SensorCycle::runOnce()`, then `DeepSleepClock::sleepFor(sleep_ms)`. `loop()` is unreachable.
- `lib/application/`: `SensorCycle` orchestrator, `SensorRegistry`, `SensorNodeConfig` (pins, timeouts, addresses).
- `lib/infrastructure/src/drivers/`: `AM2315CSensor`, `ChirpSoilSensor` — both implement `ISensorChannel`.
- `lib/infrastructure/src/platform/`: `BatteryMonitor` (also `ISensorChannel`), `GpioPowerRail`, `DeepSleepClock`, `ArduinoClock`, `SerialLogger`.
- `lib/infrastructure/src/network/`: `ZigbeeEndDeviceAdapter`, `ZigbeeReportMapper`, `ChannelMappings.hpp` (compile-time per-channel attribute table).
- `platformio.ini`: three envs — `sensor-node` (HW), `sensor-node-hwtest` (HW, drivers), `sensor-node-native` (host unit tests). Strict warnings `-Wextra -Wconversion -Wshadow -fno-rtti` are applied to project code via `extra_script.py` + per-library `library.json` `build.flags`; SDK headers stay relaxed.
- `partitions.csv`: single-app 4 MB layout, no OTA in this phase.
- `test/`:
  - `test/test_sensor_cycle/` — native env, uses `FakeSensorChannel` + `FakeZigbeeEndDevice` + `FakeClock` + `FakeLogger`.
  - `test/test_sensor_registry/` — native env, exercises `add()`, `probeAll()`, `maxWarmupMs()`, `presentMask()`.
  - `test/test_zigbee_report_mapper/` — native env.
  - `test/test_drivers/test_am2315c_sensor/`, `test_chirp_soil_sensor/`, `test_battery_monitor/`, `test_sensor_power_gate/`, `test_zigbee_end_device/` — `sensor-node-hwtest` env (require connected MCU).

**Build / test commands:**
```bash
pio run  -e sensor-node           -d firmware/sensor-node    # HW build
pio run  -e sensor-node -t upload -d firmware/sensor-node    # flash over USB-C
pio test -e sensor-node-native    -d firmware/sensor-node    # 6/6 host tests
pio test -e sensor-node-hwtest    -d firmware/sensor-node    # HW tests
```

---

## [2] Architecture — ports, services, mappers

### [2.1] Ports (interfaces in `shared/domain/src/ports/`, implementations here)

| Port               | Where the interface lives                          | Implementation                                                                          |
|--------------------|----------------------------------------------------|-----------------------------------------------------------------------------------------|
| `IZigbeeEndDevice` | `shared/domain/src/ports/IZigbeeEndDevice.hpp`     | `lib/infrastructure/src/network/ZigbeeEndDeviceAdapter.{hpp,cpp}`                       |
| `IDeepSleep`       | `shared/domain/src/ports/IDeepSleep.hpp`           | `lib/infrastructure/src/platform/DeepSleepClock.{hpp,cpp}`                              |
| `IPowerRail`       | `shared/domain/src/ports/IPowerRail.hpp`           | `lib/infrastructure/src/platform/GpioPowerRail.{hpp,cpp}` (GPIO4 p-MOSFET + `gpio_hold_en` across deep sleep) |
| `ISensorChannel`   | `shared/domain/src/ports/ISensorChannel.hpp`       | `lib/infrastructure/src/drivers/AM2315CSensor`, `.../ChirpSoilSensor`, `lib/infrastructure/src/platform/BatteryMonitor` |
| `IClock`           | `shared/domain/src/ports/IClock.hpp`               | `lib/infrastructure/src/platform/ArduinoClock.{hpp,cpp}`                                |
| `ILogger`          | `shared/domain/src/ports/ILogger.hpp`              | `lib/infrastructure/src/platform/SerialLogger.{hpp,cpp}` (signature: `info(tag, msg)` / `warn` / `error` — NOT variadic) |

`IAirSensor` / `ISoilSensor` / `IBatteryMonitor` no longer exist; all three sensor families implement `ISensorChannel`.

### [2.2] Application layer (sensor-node only, not shared)

- `lib/application/src/SensorCycle.{hpp,cpp}` — one wake cycle: `rail.on()` → wait `registry.maxWarmupMs()` → iterate Ok channels → `mapper.publish(readings, presentMask, tx_timeout_ms)` → `rail.off()` → return `zb.reportPeriodSeconds() * 1000`. Knows nothing about ZCL — only the registry and the mapper.
- `lib/application/src/SensorRegistry.{hpp,cpp}` — fixed-capacity (`kMaxSensorChannels = 8`) array of `ISensorChannel*`. `add()` (registration), `probeAll(rail, log)` (one-time boot probe — power on, probe each, power off), `maxWarmupMs()`, `presentMask()` (one bit per channel where `status() == Ok`).
- `lib/application/src/SensorNodeConfig.hpp` — `constexpr` config: pins (`kI2cSdaPin=6`, `kI2cSclPin=7`, `kSensorPowerGateGpio=4`, `kBatteryAdcGpio=0`), addresses (`kAm2315cAddress=0x38`, `kChirpAddress=0x20`), battery divider, `kZbSteeringTimeoutMs=60'000`, `kZbTxTimeoutMs=3'000`, `kFailedJoinSleepMs=1 h`, mirror of `kDefaultReportPeriodS`.

### [2.3] Infrastructure layer — ZCL mapping lives here, not in application

- `lib/infrastructure/src/network/ZigbeeReportMapper.{hpp,cpp}` — for each `SensorReading` from the registry: look up `ChannelMapping` by `id().value`, encode each `AttrMapping` via its `AttrEncoder` (free function, no state), call `zb_.reportAttribute(...)`. After all readings: publish `sensors_present_mask` on EP1 Basic@0xF001.
- `lib/infrastructure/src/network/ChannelMappings.hpp` — header-only compile-time table. One `AttrMapping[]` per channel (air → temp+humidity, soil → moisture EP1 + temp EP2, battery → pct + voltage), then one `ChannelMapping` row per channel collecting `{channel_id, expected_kind, attrs[], attr_count}`. Encoders are inline free functions (`encodeAirTemp`, `encodeSoilMoist`, etc.) using `gh::protocol::*ToZcl` helpers from `ZclIds.hpp`.

### [2.4] Adding a new sensor channel (PAR / CO₂ / 2nd Chirp / …)

See [`docs/superpowers/specs/2026-05-31-sensor-node-plugin-architecture-design.md`](../../docs/superpowers/specs/2026-05-31-sensor-node-plugin-architecture-design.md) §5 for the full rationale. In four steps:

1. **Domain side**: append a new `kSensorChannelId<Name>` constant and (if needed) a new `SensorKind` enumerator in `shared/domain/src/entities/SensorKind.hpp` and matching union member in `SensorReading.hpp`.
2. **Adapter**: create `lib/infrastructure/src/drivers/<Name>Sensor.{hpp,cpp}` (or `platform/` for non-I²C devices) implementing `ISensorChannel` — declare `id()` / `kind()` / `warmupMs()`, implement `probe()` + `read()`.
3. **Mapping**: append one `AttrMapping[]` (one row per ZCL attribute the channel emits) and one `ChannelMapping` row in `lib/infrastructure/src/network/ChannelMappings.hpp`. Encoders are tiny inline free functions next to the existing ones.
4. **Wire + test**: instantiate the adapter in `src/main.cpp` and call `registry.add(...)`. Add a native test in `test/test_<name>/` using `FakeSensorChannel`; add an hwtest if there's hardware-specific behaviour worth catching.

---

## [3] How the sensor-node delivers data to the coordinator

### [3.1] Link layer — Zigbee (IEEE 802.15.4)

- **Radio:** ESP32-C6 native 802.15.4, **not Wi-Fi**. Wi-Fi is never initialised on the sensor-node.
- **Role:** Zigbee End Device, **sleepy** (`rx_on_when_idle = false`). Between cycles the radio is off.
- **Network forming:** done by the coordinator. On first power-up the sensor-node performs `steering` (join). After pairing, the network coordinates are stored in NVS namespace `zigbee_pair` (extended PAN ID + parent short_addr). On subsequent boots re-attach takes ≤ 1 s.
- **SDK:** Espressif `esp-zigbee-lib`, invoked through the ESP-IDF component path. The PIO env already supports the `framework = arduino, espidf` mix (provided by pioarduino).

### [3.2] ZCL endpoints and clusters (standard, no custom payload)

The sensor-node declares **two endpoints**:

**Endpoint 1** (`kSensorEndpoint = 1`) — primary clusters:

| Cluster                  | ID     | Purpose                              | Attribute            |
|--------------------------|--------|--------------------------------------|----------------------|
| Basic                    | 0x0000 | manuf, model, fw version, **report_period**, **sensors_present_mask** | 0xFF00 (custom, uint32, read-write, seconds); 0xF001 (custom, uint32, read-only+reporting, bit per `SensorChannelId` — surfaced to HA as per-channel health) |
| Power Configuration      | 0x0001 | battery                              | 0x0021 BatteryPercentageRemaining (uint8 ×2 → %×2) |
| Temperature Measurement  | 0x0402 | air temperature (AM2315C)            | 0x0000 MeasuredValue (int16 centi-°C) |
| Relative Humidity        | 0x0405 | air relative humidity                | 0x0000 MeasuredValue (uint16 centi-%) |
| Soil Moisture            | 0x0408 | soil moisture                        | 0x0000 MeasuredValue (uint16 centi-%) |

**Endpoint 2** (`kSensorSoilTempEndpoint = 2`) — Chirp soil temperature:

| Cluster                  | ID     | Purpose                              | Attribute            |
|--------------------------|--------|--------------------------------------|----------------------|
| Temperature Measurement  | 0x0402 | soil temperature (Chirp)             | 0x0000 MeasuredValue (int16 centi-°C) |

EP2 uses a separate endpoint so EP1's cluster list stays interoperable with standard Zigbee/ZHA integrations (one TempMeasurement cluster per endpoint). Model id bumped to `gh-sensor-node-v2` to prevent HA from confusing EP1-only (v1) and EP1+EP2 (v2) firmware.

### [3.3] What goes over the air each cycle

On wake-up the sensor-node sends **7** individual Zigbee `Report Attributes` commands (one per attribute), followed by a read of the report period. Each report is APS-ack'd before the next is sent.

```
sensor-node                   coordinator
   │                              │
   ├── RTC timer wakeup            │
   ├── gate.on() (MOSFET)          │
   ├── SensorRegistry::probeAll()  │
   ├── warmup 1000 ms              │
   ├── SensorRegistry::readAll()   │
   │   ├── read AM2315C (~10 ms)   │
   │   ├── read Chirp   (~10 ms)   │
   │   └── read battery (~1 ms ADC)│
   ├── 802.15.4 radio ON           │
   ├── ZigbeeReportMapper::publish()│
   │   ├── Report Attr ep1 0x0402/0x0000 ─►│  value=2340  (23.40°C air temp)
   │   ├── Report Attr ep2 0x0402/0x0000 ─►│  value=1850  (18.50°C soil temp)
   │   ├── Report Attr ep1 0x0405/0x0000 ─►│  value=5620  (56.20% humidity)
   │   ├── Report Attr ep1 0x0408/0x0000 ─►│  value=4200  (42.00% soil moisture)
   │   ├── Report Attr ep1 0x0001/0x0020 ─►│  value=41    (4.1 V battery voltage)
   │   ├── Report Attr ep1 0x0001/0x0021 ─►│  value=174   (87% battery)
   │   └── Report Attr ep1 0x0000/0xF001 ─►│  value=0x07  (channels 0,1,2 ok — sensors_present_mask)
   │                              ├── update SensorCache
   ├── Read Attribute 0xFF00 ─────►│
   │                              │
   ├── ◄─ ReadAttrResponse 300     │  (or a new value if the coord wrote one)
   ├── gate.off()                  │
   ├── radio OFF                   │
   ├── esp_deep_sleep_start(300s)  │
```

### [3.4] Configurable report period

The sensor-node does not hard-code its interval — it is stored as **attribute 0xFF00 in cluster Basic** (Manufacturer Specific Attribute, uint32 seconds).

- First boot: uses the default (`kReportPeriodDefaultS = 60`, mirrored as `SensorNodeConfig::kDefaultReportPeriodS`).
- The coordinator writes its desired period via `Write Attribute 0xFF00` (`AppConfig::kReportPeriodS = 60`) the first time it sees a new sensor short-addr (`ZigbeeCoordinatorAdapter::drainPendingPeriodWrites`). The sensor-node reads `0xFF00` from its local attribute store at the end of every cycle, just before deep-sleep, so the new value takes effect on the next wake.
- Boundary values: `kReportPeriodMinS = 60`, `kReportPeriodMaxS = 3600`. The sensor-node clamps on read (`ZigbeeEndDeviceAdapter::reportPeriodSeconds`) — the coordinator also validates `[60, 3600]` before issuing the write.

### [3.5] Pairing / commissioning

First-boot behaviour (NVS `zigbee_pair` empty):
1. The sensor-node sees no saved network → enters `steering` for `kZbSteeringTimeoutMs = 60 s`. Steering matches on TC link key + channel mask; ExtPanId is NOT asserted on the joiner side (each coordinator persists its own randomised ExtPanId in `zigbee_net`).
2. The coordinator opens the permit-join window for `kInitialPermitJoinMs = 60 s` only on first network formation. On subsequent boots the window is closed — the operator opens it via `POST /api/zigbee/permit-join {"duration_s": N}`.
3. On successful join → the observed Trust Center IEEE is persisted to NVS `zigbee_pair` → one report cycle runs → deep sleep.

If `zb.start()` fails the device logs the failure and deep-sleeps for `kFailedJoinSleepMs = 1 h`. A `ZigbeeTrustCenterMismatch` result additionally calls `ZigbeeEndDeviceAdapter::clearPairingNvs()` so the next boot does fresh steering against whatever TC IEEE is now on the channel (covers the case where the coordinator was re-flashed and its `zigbee_net` rotated).

### [3.6] What is NOT transmitted

- **Wi-Fi credentials, MQTT credentials** — the sensor-node has none; Wi-Fi is off.
- **OTA** — not implemented (USB-C re-flash only). Optional Phase D.

**Decided in Phase B (now wire-stable):**

- **Raw Chirp capacitance is what goes over the air**, scaled into the ZCL 0..10000 range by `gh::protocol::soilRawToZcl()` (`raw * 10000 / 1023`, saturating). The coordinator's `SoilNormalizer` converts that back to a true `%` using calibration in NVS `soil_calib`. Calibration **lives on the coordinator only** — the sensor-node is a dumb sender. `raw_dry` / `raw_wet` are not flashed into the sensor-node firmware.
- **Soil temperature is on a separate endpoint** (EP2, cluster 0x0402). EP1 carries everything else. Model id bumped to `gh-sensor-node-v2`.
- **Battery voltage is transmitted** alongside `BatteryPercentageRemaining` (ZCL `BatteryVoltage = 0x0020`, units of 100 mV). The coordinator publishes both as separate HA Discovery entities (`device_class: battery` / `voltage`).

#### Chirp calibration values for the current unit

Measured during hardware bring-up on 2026-05-22 (on a throwaway ESPHome sketch, since removed) with this specific sensor instance @3V3:

| Parameter  | Value   | Method                                                |
|------------|---------|-------------------------------------------------------|
| `raw_dry`  | 249     | probe in air, 30 s stabilisation                      |
| `raw_wet`  | 489     | probe in a glass of tap water up to the marker line   |

`raw_dry=249` matches the Catnip Electronics reference @3V3 (~250 for dry soil, see `archive/i2c-moisture-sensor-master/Soil Moisture Sensor Calibration.pdf`). `raw_wet=489` is a clone with a narrower upper range (the original reaches ~650–700 in saturated soil). The linear normalisation `pct = (raw - 249) / 2.4` works; in freshly-watered soil the ceiling is ~85–90 % (soil+water has a lower ε than pure water).

These measured values live in the coordinator's NVS `soil_calib` (entered at provisioning / via REST), **not** in `SensorNodeConfig` — the sensor-node ships raw capacitance. The firmware *default* in `AppConfig::kDefaultSoilCalibration` is a generic `raw_dry=300 / raw_wet=700` fallback used only when NVS is empty — it intentionally does **not** encode this unit's `249/489`. The NVS override means a sensor swap does not require re-flashing. Canonical: [`docs/hardware/reference/canonical-values.md#calibration`](../../docs/hardware/reference/canonical-values.md#calibration).

---

## [4] Build & flash commands

```bash
# Build (no flash) — under restored -Wextra -Wconversion -Wshadow -fno-rtti
pio run -e sensor-node -d firmware/sensor-node

# Flash over USB-C
pio run -e sensor-node -t upload -d firmware/sensor-node

# Monitor (Serial @115200)
pio device monitor -e sensor-node -d firmware/sensor-node

# Host unit tests (13 cases across SensorCycle, SensorRegistry, ZigbeeReportMapper)
pio test -e sensor-node-native -d firmware/sensor-node

# Driver HW tests — require connected MCU
pio test -e sensor-node-hwtest -d firmware/sensor-node
```

Strict warnings only apply to our `lib/` + `src/` code (via `extra_script.py` + per-library `library.json` `build.flags`). Vendor framework headers are not touched.

### Flashing a bricked board

If the board does not respond to flashing:
1. Hold the `BOOT` button.
2. Briefly press `RESET`.
3. Release `BOOT` → the board is in Firmware Download mode.
4. Run `pio run -t upload`.

After a successful flash the native USB should bring up the USB-CDC port automatically.

---

## [5] Decisions and remaining open questions

Resolved during Phase B (kept here as a short audit trail; details in §3.6 + spec):

- **Pinout**: SuperMini clone matches DevKitM-1 for ADC1 (GPIO0–6) and LP_I2C (GPIO6/7) — verified on bench.
- **`esp-zigbee-lib` via pioarduino arduino+espidf mix**: works stably; `ZigbeeEndDeviceAdapter` is in production.
- **Battery SoC**: piecewise-linear from a LiPo discharge curve (`BatteryMonitor::voltageToSocPct`) — 4.20 V = 100 %, 3.70 V = 50 %, 3.40 V = 5 %, 3.20 V = 0 %. No fuel-gauge IC.
- **Report period attribute**: custom `0xFF00` on Basic. HA sees the coordinator as an MQTT device — ZHA compatibility is not a goal for this build.
- **Soil moisture wire format**: raw Chirp capacitance scaled to ZCL 0..10000 (see `soilRawToZcl`). The coordinator's `SoilNormalizer` does the `raw → %` mapping with NVS-stored calibration.

Still open:

| #  | Question                                                          | Plan                                              |
|----|-------------------------------------------------------------------|---------------------------------------------------|
| Q1 | USB-CDC for `Serial` logs over native USB on SuperMini (no UART bridge on this board). Currently `platformio.ini` does **not** define `-DARDUINO_USB_CDC_ON_BOOT=1 -DARDUINO_USB_MODE=1`, so logs go nowhere unless a UART pad is wired manually. | Add the CDC flags + verify enumeration when we need on-board log diagnostics post-Phase B. Low priority — current bring-up uses external UART for diagnostics. |
| Q2 | Stuck-BUSY Chirp recovery — current `ChirpSoilSensor::waitNotBusy_()` calls `bus_.end(); bus_.begin();` with no args, relying on Wire to reuse the last pins/freq. Confirm this round-trip on a real stuck-BUSY case on the bench. | Test during the next bench cycle; surface a real failure path before adding fallback behaviour. |

---

## [6] Quick cheat-sheet — where to put code

| I want to add / change…                                | Where                                                                                                                                                                            |
|--------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| New business scenario (`SensorCycle` flow)             | `lib/application/src/SensorCycle.{hpp,cpp}` (+ test in `test/test_sensor_cycle/`)                                                                                                |
| New port (visible to both firmwares)                   | `shared/domain/src/ports/`                                                                                                                                                       |
| New hardware sensor                                    | implement `ISensorChannel` in `lib/infrastructure/src/drivers/` (or `platform/` for non-I²C); append one row to `lib/infrastructure/src/network/ChannelMappings.hpp`; `registry.add(...)` in `src/main.cpp`; test with `FakeSensorChannel` |
| New ZCL attribute on an existing channel               | append an `AttrMapping` to the channel's `AttrMapping[]` in `ChannelMappings.hpp`; add an `encode<Name>` free function alongside the existing encoders                          |
| Re-tune pins / addresses / timeouts                    | `lib/application/src/SensorNodeConfig.hpp`                                                                                                                                       |
| Cluster IDs / attribute IDs / ZCL helpers              | shared: `shared/protocol/src/ZclIds.hpp`                                                                                                                                         |
| Low-level ESP-IDF code (deep sleep, RTC, gpio_hold)    | `lib/infrastructure/src/platform/`                                                                                                                                               |
| Zigbee state-machine / TC IEEE / NVS pairing logic     | `lib/infrastructure/src/network/ZigbeeEndDeviceAdapter.{hpp,cpp}`                                                                                                                |
| Test for business logic                                | `test/test_<name>/` env `sensor-node-native`                                                                                                                                     |
| Test for a hardware driver                             | `test/test_drivers/test_<name>/` env `sensor-node-hwtest`                                                                                                                        |

---

## [7] Things to NEVER do

- **Never bring up Wi-Fi.** The sensor-node runs without Wi-Fi forever — it's a battery device. `WiFi.mode(WIFI_OFF)` + `btStop()` are called BEFORE the radio comes up because Wi-Fi shares the RF front-end with 802.15.4 on ESP32-C6.
- **Never use `Serial.print` in the hot path.** Use `SerialLogger` (`log.info("tag", "msg")`); the logger is gated and won't smear power profile. Every wake-up costs precious milliseconds.
- **Never call `delay()` longer than 10 ms.** Long delays burn battery in active mode. Use `IDeepSleep::sleepFor()`. (The one exception is the rail warmup wait inside `SensorCycle::runOnce()` — bounded by `registry.maxWarmupMs()` and intentional.)
- **Never store calibration parameters here.** `raw_dry` / `raw_wet` live on the coordinator (NVS `soil_calib`). The sensor-node is a dumb sender.
- **Never use `new` / `malloc` in `SensorCycle::runOnce()`.** All buffers static (embedded rule from global CLAUDE.md §1). The cycle's `SensorReading readings[kMaxReadings]` is a stack array; the ZCL encoders write into caller-provided small buffers, never heap.
- **Never call `ZigbeeEndDeviceAdapter::start()` twice in one boot.** It heap-allocates inside `Preferences::begin()`; a second call is the only place we'd violate the "no malloc after init" rule. The adapter has a `s_started` guard that returns Ok on the second call and logs a warning — but don't rely on it.
- **Do not touch `GPIO12` / `GPIO13`** — native USB (debug / re-flash on SuperMini, which has no separate UART bridge).
- **Do not use `GPIO8` for your own load** (WS2812 sits on it, plus it's a strapping pin).
- **GPIO4 needs the external 100 kΩ pull-up to VBAT** before powering the board. Without it the MOSFET briefly conducts during the high-Z boot window. `gpio_hold_en` only latches the pin *after* boot — it can't fix the strapping-pin window.
