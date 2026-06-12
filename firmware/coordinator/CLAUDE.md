# CLAUDE.md — `firmware/coordinator/`

> This file is a local guide for AI / engineers working on the coordinator firmware. Chip-level facts (ESP32-C6 capabilities, strapping pins, ADC1 constraints, "which pin should I use" decision tree) — `../CLAUDE.md` (`firmware/CLAUDE.md`). Clean architecture, code conventions, error handling, security — root `/Users/bladerunner/Work/personal_projects/green-house-esp32/CLAUDE.md`. Full architectural design — `docs/superpowers/specs/2026-05-17-split-coordinator-sensor-node-design.md`.

---

## [0] What this even is

The coordinator is the **mains-powered brain** of the greenhouse on the ESP32-C6-DevKitM-1. Its job: form a small Zigbee network and accept sensor reports from the sensor-node, publish the latest values to Home Assistant via MQTT (HA Discovery), accept pump on/off commands from HA, and drive a relay-based water pump under firm safety rules (max-runtime cutoff, float-switch dry-run guard, watchdog on sensor freshness).

**Target hardware:**
- **Board:** ESP32-C6-DevKitM-1 (ESP32-C6-MINI-1 module, ESP32-C6FH4 chip, 4 MB flash, on-board PCB antenna). Hardware reference — [`../../archive/esp-dev-kits-en-master-esp32c6.pdf`](../../archive/esp-dev-kits-en-master-esp32c6.pdf). Board-level details (components, GPIO map of headers J1/J3, hard constraints specific to this board) — §0.1 below.
- **Power:** 5 V from a 12V→5V buck wired to VIN (J1.14). Mains-powered — no battery.
- **Connectivity:** Wi-Fi 6 (STA) → MQTT (HA Discovery via `PubSubClient`) + AP-mode captive portal for provisioning (`ESPAsyncWebServer`). REST API is planned for Phase C. **Zigbee** Coordinator role over IEEE 802.15.4 (esp-zigbee-sdk).
- **Storage:** NVS (`Preferences`) for Wi-Fi / MQTT creds, soil calibration, admin password hash, Zigbee network state. LittleFS slot reserved for Phase C (telemetry buffering / OTA staging — not used yet).
- **Partitions:** [`partitions.csv`](partitions.csv) — currently single-factory app (spec promises dual-OTA + LittleFS in Phase C; reconcile at that time).

### Coordinator pinout (DevKitM-1)

The canonical wiring source is [`../../COMPONENTS.md`](../../COMPONENTS.md) `> ESP32-C6 pinout`. Duplicated below with rationale so the AI sees it alongside the code.

| GPIO  | Header | Function                                             | Note                                                                                                       |
|-------|--------|------------------------------------------------------|------------------------------------------------------------------------------------------------------------|
| 6     | J1.10  | I²C SDA (Phase B: not used — sensors live on the sensor-node) | `LP_I2C_SDA`, kept available for a future local sensor; 4.7 kΩ pull-up to 3V3 still wired                  |
| 7     | J1.11  | I²C SCL (Phase B: idle)                              | `LP_I2C_SCL`, 4.7 kΩ pull-up                                                                               |
| 8     | J1.9   | Onboard WS2812 status LED                            | strapping pin, board-driven — do **not** repurpose for external loads                                       |
| 9     | J3.11  | BOOT button (onboard, dual-use as a provisioning trigger) | `kBootButtonGpio` in `AppConfig`; **strapping pin** — no external load (must be high-Z during reset)        |
| 14    | J1.12  | Float switch input, `INPUT_PULLUP`, active-low       | only clean digital-only pin on J1; not strapping, not ADC, not LP                                          |
| 18    | J3.9   | Relay CH1 → pump, output                              | digital-only, not strapping; active-high (configurable via `kPumpRelayActiveHigh`)                         |
| 19    | J3.8   | Relay CH2 → spare, output                             | reserved                                                                                                   |

**Spare pins (room for expansion):**
- ADC-capable: `GPIO0..GPIO5` (CH0..CH5) — GPIO6 is occupied by I²C SDA, so ADC1_CH6 is unavailable.
- Digital-only: `GPIO20..GPIO23` on J3 (SDIO_DATA0..3 aliases, if SDIO is not used).
- LP-capable: `GPIO0..GPIO5` (plus GPIO6/7 are already used for LP_I²C).

**Reserved / do not touch:** `GPIO9` (BOOT button + strapping), `GPIO12/13` (native USB), `GPIO15` (strapping), `GPIO16/17` (HP UART0 routed through the USB-UART bridge — `Serial` logs). `GPIO10/11` are not physically exposed on the MINI-1 module.

---

## [0.1] DevKitM-1 board-level details

> Chip-level facts (ESP32-C6 subsystems, ADC1, LP, strapping, "which pin to use") are in [`../CLAUDE.md`](../CLAUDE.md) — they apply equally to DevKitM-1 and SuperMini. This section is **DevKitM-1 only**.

### Board components

| Component | Purpose |
|---|---|
| 5V → 3.3V LDO         | powers the module from USB / 5V |
| USB-to-UART Bridge    | up to 3 Mbps; connected to U0TXD/U0RXD (GPIO16/17) — this is why `Serial.print` "just works" on DevKitM-1 without USB-CDC |
| ESP32-C6 USB Type-C   | native USB (D+ = GPIO13, D− = GPIO12) — flash / USB-CDC / USB-JTAG debug |
| USB Type-C to UART    | power + flash via the UART bridge |
| Boot Button           | hold Boot + press Reset → Firmware Download mode. Also used by the firmware as the provisioning trigger (`kBootButtonGpio = 9`). |
| Reset Button          | system reset |
| RGB LED               | addressable (WS2812-compatible), on GPIO8 |
| J5 jumper             | breaks the module power rail → ammeter current measurement |

**Power options (mutually exclusive):** USB Type-C (default) · 5V + GND headers · 3V3 + GND headers. **EN/RST pin:** High = power up, Low = power down.

### GPIO map of DevKitM-1 (15 pins per side)

```
J1 (left side)                                J3 (right side)
 1  3V3    power                                1  GND
 2  RST    chip enable                          2  GPIO16  U0TXD / FSPICS0     ← Serial logs
 3  GPIO2  LP_GPIO2  LP_UART_RTS  ADC1_2 FSPIQ  3  GPIO17  U0RXD / FSPICS1     ← Serial logs
 4  GPIO3  LP_GPIO3  LP_UART_CTS  ADC1_3        4  GPIO23  SDIO_DATA3
 5  GPIO4  MTMS  LP_GPIO4  LP_UART_RXD  ADC1_4  5  GPIO22  SDIO_DATA2
 6  GPIO5  MTDI  LP_GPIO5  LP_UART_TXD  ADC1_5  6  GPIO21  SDIO_DATA1 / FSPICS5
 7  GPIO0  LP_GPIO0  LP_UART_DTR  ADC1_0        7  GPIO20  SDIO_DATA0 / FSPICS4
 8  GPIO1  LP_GPIO1  LP_UART_DSR  ADC1_1        8  GPIO19  SDIO_CLK   / FSPICS3
 9  GPIO8  (strapping, drives RGB LED)          9  GPIO18  SDIO_CMD   / FSPICS2
10  GPIO6  MTCK  LP_GPIO6  LP_I2C_SDA  ADC1_6  10  GPIO15  (strapping)
11  GPIO7  MTDO  LP_GPIO7  LP_I2C_SCL          11  GPIO9   (strapping, BOOT button)
12  GPIO14                                     12  GND
13  GND                                        13  GPIO13  USB_D+
14  5V     power                               14  GPIO12  USB_D−
15  GND                                        15  GND
```

> On DevKitM-1, the only truly "free" digital-only pin on J1 is **GPIO14** (J1.12) — not strapping, not ADC, not LP, no alt functions. This is why the float switch lives there.

### Board-specific hard constraints

1. **HP UART0 (logs via `Serial.print`) is occupied by the USB-UART bridge on GPIO16/17.** If you need these pins for your own peripherals — reassign `Serial` to a different UART via `Serial.setPins()` or route logs through USB-CDC. The current firmware uses the bridge (no USB-CDC flags in `platformio.ini`).
2. **Both USB-C ports physically present** (UART bridge + native USB). The firmware can be flashed via either; logs go to the UART bridge port by default.

---

## [1] Current state — Phase D complete (multi-node)

The coordinator is the **functionally rich half** of the project. As of `8084e13` (multi-node cutover) + `3fe040f` (SPA rewrite) it builds for hardware and is wired end-to-end on the multi-node pipeline: Zigbee in → `ZigbeeReportRouter` → `NodeRegistry` + `HistoryStore` + `AnalyticsUploader` bridge → MQTT/HA Discovery + REST API + `IrrigationService`. The legacy single-node pipeline (`SensorCache`, `SensorAvailabilityWatchdog`, `HistoryRingBuffer`, single-node `TelemetryPublisher`, `MqttTelemetrySink`, single-node `IrrigationService`, single-node `HomeAssistantDiscovery`, single-node `RestApi` routes) has been removed. Tag: `v2.0.0-multinode`.

**What is implemented and working:**

| Subsystem                      | Class / file                                                                 | Status |
|--------------------------------|------------------------------------------------------------------------------|--------|
| Composition root               | [`src/main.cpp`](src/main.cpp)                                               | ✅     |
| Wi-Fi provisioning (BOOT button → captive AP)  | [`WifiProvisioner`](lib/application/src/WifiProvisioner.cpp), [`ProvisioningWebServer`](lib/infrastructure/src/network/ProvisioningWebServer.cpp) | ✅     |
| Wi-Fi STA connect              | [`WifiStaAdapter`](lib/infrastructure/src/network/WifiStaAdapter.cpp)        | ✅     |
| NVS persistence (Wi-Fi / MQTT / soil calibration / aliases) | [`NvsWifiCredsStore`](lib/infrastructure/src/persistence/NvsWifiCredsStore.cpp), `NvsMqttCredsStore`, `NvsSoilCalibrationStore`, `NvsNodeAliasStore` | ✅     |
| MQTT client (PubSubClient)     | [`MqttClient`](lib/infrastructure/src/network/MqttClient.cpp)                | ✅     |
| MQTT command router (pump ON/OFF) | [`MqttCommandRouter`](lib/presentation/src/MqttCommandRouter.cpp)            | ✅     |
| Pump relay driver              | [`RelayPump`](lib/infrastructure/src/drivers/RelayPump.cpp)                  | ✅     |
| **Node registry** (IEEE-keyed, in-memory, snapshot for readers)            | [`InMemoryNodeRegistry`](lib/infrastructure/src/registry/InMemoryNodeRegistry.cpp) | ✅ |
| **History store** (per-node, per-channel ring buffer, last 24 h)           | [`InMemoryHistoryStore`](lib/infrastructure/src/registry/InMemoryHistoryStore.cpp) | ✅ |
| **Zigbee binding table** (declarative cluster → callback wiring)           | [`ZigbeeBindingTable`](lib/infrastructure/src/network/ZigbeeBindingTable.cpp) | ✅ |
| **Zigbee report router** (ZCL attr → SensorKind → registry + history + analytics) | [`ZigbeeReportRouter`](lib/application/src/zigbee/ZigbeeReportRouter.cpp) | ✅ |
| **Channel → telemetry mapper** (quantity_code + value formatting per channel) | [`ChannelToTelemetryMapper`](lib/application/src/telemetry/ChannelToTelemetryMapper.cpp) | ✅ |
| **IrrigationService** (auto + manual modes, soil quorum, safety lock)    | [`IrrigationService`](lib/application/src/irrigation/IrrigationService.cpp) | ✅ |
| **Node prune service** (drop offline nodes past TTL)                       | [`NodePruneService`](lib/application/src/node/NodePruneService.cpp) | ✅ |
| **TelemetryPublisher** (per-node retained + change-only state topics)    | [`TelemetryPublisher`](lib/application/src/telemetry/TelemetryPublisher.cpp) | ✅ |
| **HA Discovery** (one HA device per node, channel-driven entities)      | [`HomeAssistantDiscoveryService`](lib/presentation/src/HomeAssistantDiscoveryService.cpp) | ✅ |
| **V1 MQTT purge** (one-shot retained-cleanup pass, NVS-gated)              | [`V1MqttPurge`](lib/application/src/telemetry/V1MqttPurge.cpp) | ✅ |
| **Esp32 Zigbee network adapter** (esp-zigbee-sdk wrapper, TC link key, ExtPanId, min-LQI) | [`Esp32ZigbeeNetwork`](lib/infrastructure/src/network/Esp32ZigbeeNetwork.cpp), [`ZigbeeCoordinatorAdapter`](lib/infrastructure/src/network/ZigbeeCoordinatorAdapter.cpp) | ✅ |
| **REST API — nodes list**                                                   | [`RestNodesRoutes`](lib/presentation/src/RestNodesRoutes.cpp) | ✅ |
| **REST API — node alias (rename)**                                          | [`RestNodeAliasRoutes`](lib/presentation/src/RestNodeAliasRoutes.cpp) | ✅ |
| **REST API — node delete (two-step)**                                       | [`RestNodeDeleteRoutes`](lib/presentation/src/RestNodeDeleteRoutes.cpp) | ✅ |
| **REST API — history (per-node, per-channel)**                              | [`RestHistoryRoutes`](lib/presentation/src/RestHistoryRoutes.cpp) | ✅ |
| **REST API — pump (manual override)**                                       | [`RestPumpRoutes`](lib/presentation/src/RestPumpRoutes.cpp) | ✅ |
| **REST API — status (device + Zigbee + connectivity)**                      | [`RestStatusRoutes`](lib/presentation/src/RestStatusRoutes.cpp) | ✅ |
| **REST API — config (snapshot + partial update)**                           | [`RestConfigRoutes`](lib/presentation/src/RestConfigRoutes.cpp) | ✅ |
| **REST API — Zigbee permit-join**                                           | [`RestZigbeeRoutes`](lib/presentation/src/RestZigbeeRoutes.cpp) | ✅ |
| **REST API — auto-water policy**                                            | [`RestAutoWaterRoutes`](lib/presentation/src/RestAutoWaterRoutes.cpp) | ✅ |
| Soil normaliser (raw → %, calibratable)  | [`SoilNormalizer`](../shared/application/src/SoilNormalizer.cpp)          | ✅     |
| Watering policy (pluggable; consumed by `IrrigationService`)             | [`WateringPolicy`](../shared/domain/src/policies/WateringPolicy.hpp)      | ✅     |
| JSON telemetry formatter        | [`JsonTelemetryFormatter`](../shared/application/src/JsonTelemetryFormatter.cpp) | ✅     |
| **D-1 analytics path** (LittleFS queue + HTTPS POST batch every 15 min, fed via `ZigbeeReportRouter` bridge) | [`AnalyticsUploader`](lib/application/src/AnalyticsUploader.cpp), [`LittleFsTelemetryQueue`](lib/infrastructure/src/persistence/LittleFsTelemetryQueue.cpp), [`EspHttpsClient`](lib/infrastructure/src/network/EspHttpsClient.cpp), [`NvsAnalyticsConfigStore`](lib/infrastructure/src/persistence/NvsAnalyticsConfigStore.cpp) | ✅ |
| **D-2 pairing client** (Hue-style 6-digit code → hub mints api_key) | [`EspPairingClient`](lib/infrastructure/src/network/EspPairingClient.cpp), [`IPairingClient`](../shared/domain/src/ports/IPairingClient.hpp) | ✅ |

**Build & test status (as of `8084e13`):**

```
pio test -e coordinator-native -d firmware/coordinator     # all green
pio run  -e coordinator        -d firmware/coordinator     # SUCCESS
```

**Hardware verification (pending the user):**

- Task 35 — multi-node hardware smoke (pairing + per-node telemetry + REST API + HA Discovery).
- Task 43 — manual + auto irrigation hardware smoke.

**What is intentionally NOT done yet:**

- **Dual-OTA partitions + OTA mechanism** — the spec promises "dual-OTA + LittleFS" but `partitions.csv` is currently single-factory (see §0 above). Either reconcile this in a future Phase or amend the spec.
- **`SafetyLocked` explicit unlock** — `IrrigationService::requestOff` currently clears the lock; a deliberate `unlock()` command is on the long-term TODO list.

---

## [2] How data flows end-to-end

```
sensor-node(s)               coordinator                                  MQTT broker             Home Assistant
   │                            │                                              │                       │
   │  Zigbee Report Attrs       │                                              │                       │
   ├───────────────────────────►│  Esp32ZigbeeNetwork → ZigbeeCoordinatorAdapter                       │
   │  (per-node channels,       │   → ZigbeeReportRouter (ZclSensorMapper + ChannelAttrTable)         │
   │   every ~300 s)            │      ├─→ InMemoryNodeRegistry.touch / record(channel, value)         │
   │                            │      ├─→ InMemoryHistoryStore.append(ieee, channel, value, ts)      │
   │                            │      └─→ AnalyticsUploader bridge (LittleFS queue + HTTPS batch)    │
   │                            │                                              │                       │
   │                            │  TelemetryPublisher tick (10 s):            │                       │
   │                            │   snapshot NodeRegistry → reconcile per-node retained/change-only   │
   │                            │   via ChannelToTelemetryMapper                                      │
   │                            ├─────────────────────────────────────────────►│ greenhouse/<dev>/nodes/<ieee>/online (retained)
   │                            ├─────────────────────────────────────────────►│ greenhouse/<dev>/nodes/<ieee>/present_mask (retained)
   │                            ├─────────────────────────────────────────────►│ greenhouse/<dev>/nodes/<ieee>/rssi_dbm
   │                            ├─────────────────────────────────────────────►│ greenhouse/<dev>/nodes/<ieee>/<quantity_code>
   │                            ├─────────────────────────────────────────────►│ greenhouse/<dev>/pump/state (retained)
   │                            │                                              ├──────────────────────►│ updates entities
   │                            │                                              │                       │
   │                            │  on MQTT (re)connect (also gated):           │                       │
   │                            │   HomeAssistantDiscoveryService.reconcile() │                       │
   │                            │    (one HA "device" per node, channel-driven entities)              │
   │                            │   V1MqttPurge (one-shot, NVS-gated) clears retained v1 topics       │
   │                            ├─────────────────────────────────────────────►│ homeassistant/<comp>/<dev>_<ieee>_<chan>/config (retain)
   │                            │                                              │                       │
   │                            │                                              │ greenhouse/<dev>/pump/cmd ◄─┤  user toggles switch
   │                            │  MqttCommandRouter ON / OFF                  │                       │
   │                            │   → IrrigationService.requestOn/Off (manual mode)                 │
   │                            │                                              │                       │
   │                            │  IrrigationService tick (1 Hz):            │                       │
   │                            │   ─ auto mode: average across fresh soil nodes with min_fresh_sources quorum
   │                            │   ─ manual mode: respect REST/MQTT override                         │
   │                            │   ─ max-runtime cutoff (20 s)                │                       │
   │                            │   ─ float-switch dry-run guard               │                       │
   │                            │   ─ SafetyLocked on either                    │                       │
   │                            │                                              │                       │
   │                            │  NodePruneService tick: drop nodes past offline TTL                  │
```

### MQTT topic schema (per-node)

`<dev>` is the coordinator's 6-hex device id (derived from the MAC). `<ieee>` is the sensor-node's 16-hex IEEE long address (colon-free).
`<quantity_code>` is one of: `temp_c | humidity_pct | moisture_pct | soil_temp_c | pct | voltage_v`.

| Direction | Topic                                                   | Retain | Cadence                  | Payload (example)              |
|-----------|---------------------------------------------------------|--------|--------------------------|--------------------------------|
| ↑ pub     | `greenhouse/<dev>/nodes/<ieee>/online`                  | **yes** | change-only             | `"true"` / `"false"`           |
| ↑ pub     | `greenhouse/<dev>/nodes/<ieee>/present_mask`            | **yes** | change-only             | `"0x07"`                       |
| ↑ pub     | `greenhouse/<dev>/nodes/<ieee>/proto_version`           | **yes** | change-only             | `"1"`                          |
| ↑ pub     | `greenhouse/<dev>/nodes/<ieee>/rssi_dbm`                | no      | every tick              | `"-52"`                        |
| ↑ pub     | `greenhouse/<dev>/nodes/<ieee>/<quantity_code>`         | no      | change-only (±0.01)     | `"22.50"`                      |
| ↑ pub     | `greenhouse/<dev>/pump/state`                           | **yes** | change-only             | `"ON"` / `"OFF"` / `"LOCKED"`  |
| ↑ pub     | `homeassistant/<component>/<dev>_<ieee>_<chan>/config`  | **yes** | on (re)connect           | HA Discovery JSON              |
| ↓ sub     | `greenhouse/<dev>/pump/cmd`                             | —       | —                        | `"ON"` / `"OFF"`               |

`device_id` (`<dev>`) is hex-only. Components prepend `greenhouse_` / `greenhouse/` themselves.

### IrrigationService safety state machine

```
                requestOn() / auto-policy fires      tick() | runtime >= max OR floatDry
   ┌──────────┬─────────────────────────────────►┌─────────────┬──────────────────────────►┌──────────────┐
   │   Off    │                                   │   Running   │                            │ SafetyLocked │
   └──────────┴◄────────────────────────────────┬─└─────────────┴◄─────────────────────────┬─└──────────────┘
              requestOff()                        requestOff() (clears the lock too — see TODO)
```

- **Off** → **Running** (auto): only if ≥ `min_fresh_sources` soil nodes are fresh AND their average moisture is under threshold AND the float switch is wet.
- **Off** → **Running** (manual): explicit `requestOn()` from REST/MQTT, still gated by float switch.
- **Running** → **SafetyLocked**: when `runtime_ms >= kPumpMaxRuntimeMs` (default 20 s) OR `floatSwitch.isDry()`.
- **SafetyLocked** → **Off**: currently any `requestOff()` clears the lock. Deliberate `unlock()` is on the TODO list (a paranoid user might want a manual ack).

---

## [3] Composition root walk-through

`src/main.cpp` is the only place that allocates (always `static` inside `runOperational()`) and creates FreeRTOS tasks. The flow:

1. **`setup()`** — `Serial.begin(115200)`, `Wire.begin(SDA=6, SCL=7)`, log start, then `runOperational(...)`.
2. **`runOperational()`** wires the dependency graph:
   - NVS stores: `NvsWifiCredsStore`, `NvsMqttCredsStore`, `NvsSoilCalibrationStore`, `NvsNodeAliasStore`, `NvsAutoWaterConfigStore`, `NvsAnalyticsConfigStore`, `NvsAdminCredsStore`, `NvsZigbeeNetStore`.
   - Provisioning: `GpioButton{kBootButtonGpio}` + `WifiStaAdapter` → `WifiProvisioner`. If credentials are missing **or** the user holds BOOT for 3 s on power-up → captive AP + `ProvisioningWebServer` until creds saved, then `esp_restart()`.
   - Network: `WifiStaAdapter::connect(timeout=30 s)` → on success, instantiate `MqttClient` (single-instance, asserted in ctor) and call `connect()`.
   - **Multi-node domain glue:** `Esp32ZigbeeNetwork` + `ZigbeeCoordinatorAdapter` + `ZigbeeBindingTable` → `ZigbeeReportRouter` (parses ZCL via `ZclSensorMapper` + `ChannelAttrTable`, fans out to `InMemoryNodeRegistry`, `InMemoryHistoryStore`, and the `AnalyticsUploader` bridge). `SoilNormalizer` (NVS calibration) sits inside `ChannelToTelemetryMapper`.
   - Application: `IrrigationService` (pump + clock + float switch + log + auto-water policy reading `NodeRegistry` snapshots), `NodePruneService`, `TelemetryPublisher`, `MqttCommandRouter`, `V1MqttPurge` (one-shot retained-cleanup pass).
   - Presentation: `HomeAssistantDiscoveryService` + the nine `Rest*Routes` modules (Status / Nodes / NodeAlias / NodeDelete / History / Pump / Config / Zigbee / AutoWater) plus `RestApi` facade.
   - On MQTT (re)connect → `HomeAssistantDiscoveryService::reconcile()` (per-node retained discovery configs, stack-only). `V1MqttPurge` runs once.
3. **FreeRTOS tasks** (created last, named, stacks ≥20 % headroom):
   - `mqtt_task` — `MqttClient::loop()` + reconnect ladder.
   - `telemetry_task` — fires `TelemetryPublisher::tick()` at the configured period; also drives `HomeAssistantDiscoveryService::reconcile()`.
   - `irrigation_task` — `IrrigationService::tick()` at 1 Hz (safety state machine + auto-water policy).
   - `node_prune_task` — drops offline nodes past TTL.

> The task-to-object bridge is a single file-scope `struct TaskCtx s_ctx{}` populated in `runOperational()` before any `xTaskCreate`. Each operational task receives `&s_ctx` via `pvParameters` and dereferences the fields it needs (the C-style callback API of `xTaskCreate` + PubSubClient still forces *some* file-scope handle; `TaskCtx` keeps it to exactly one).

---

## [4] Build / test / flash

```bash
# Hardware build (no flash)
pio run -e coordinator -d firmware/coordinator

# Flash over USB-C
pio run -e coordinator -t upload -d firmware/coordinator

# Serial monitor (@115200)
pio device monitor -e coordinator -d firmware/coordinator

# Host unit tests (Unity, native)
pio test -e coordinator-native -d firmware/coordinator

# Driver HW tests (NVS, etc. — needs a connected MCU)
pio test -e coordinator-hwtest -d firmware/coordinator
```

### Provisioning the board for the first time

1. Flash the firmware.
2. On first boot, no Wi-Fi credentials are stored → the board comes up as an AP named `Greenhouse-Setup-XXXX` (WPA2 passphrase derived from MAC, printed on Serial at boot).
3. Join the AP → captive portal opens automatically (or browse `http://192.168.4.1/`).
4. Fill the form:
   - **Wi-Fi:** SSID + password (required)
   - **MQTT:** host / port / user / password (required for HA Discovery)
   - **Soil calibration:** raw_dry / raw_wet (optional; skipped if both omitted)
   - **Admin:** user/password/confirm (required, see [§ Admin credentials](#admin-credentials-phase-c-web-access-hardening))
   - **Analytics hub (optional, D-2):** see [§ Analytics + pairing flow](#analytics--pairing-flow-d-2) below
5. The board saves to NVS and reboots into operational mode.

To re-enter provisioning manually: hold the onboard BOOT button (GPIO9) for ≥ 3 s right after a reset.

### Analytics + pairing flow (D-2)

If the operator wants the coordinator to ship telemetry to the FastAPI hub (long-term storage + admin REST), the captive portal asks for two extra fields:

- **Backend URL** — e.g. `http://192.168.1.42:8000/ingest` (laptop IP + `/ingest` path). Empty → analytics path stays dormant.
- **Pairing code** — 6-digit numeric code obtained from the hub admin UI / CLI via `POST /api/pairing/open`.

When both are filled, the captive portal handler does the **Hue-style pairing flow synchronously inside the form POST**:

```
[Save form] → save Wi-Fi creds to NVS → switch to WIFI_AP_STA →
  WiFi.begin(...) wait ≤30 s for STA →
    POST <hub_url>/api/pairing/claim
      body: {claim_code, device_id="gh-<6hex>", mac, fw_version, profile_id="gh-coordinator-v1"}
      → 200: hub mints + returns {api_key, device_id}
           ↳ persist {hub_url, api_key, flush_period_s=900, insecure_tls=false}
             to NVS namespace `analytics`
           ↳ render success page → restart into operational mode
      → 410: "pairing code expired or unknown"
      → 409: "device already registered (admin must revoke first)"
      → 422: "profile not supported by hub (upgrade hub)"
      → http transport: "hub unreachable"
```

The `analytics_code` form field is **transient** — never stored. Only the api_key returned by the hub lands in NVS. The 6-digit code is single-use (consumed by `pairing_windows.consumed_by` on the hub side).

**Wire contract owners:**
- `EspPairingClient` ([`lib/infrastructure/src/network/EspPairingClient.cpp`](lib/infrastructure/src/network/EspPairingClient.cpp)) builds the URL + JSON body via stack buffers, calls `EspHttpsClient::postJsonWithBody`, parses `{api_key}` via ArduinoJson's `StaticJsonDocument<256>`.
- Profile identity is a compile-time constant in `ProvisioningWebServer::handlePost` (`"gh-coordinator-v1"`). When the firmware grows new sensors / a new ZCL endpoint, bump the profile string AND ship a hub Alembic migration adding the new `device_profiles` row (see [`services/hub/CLAUDE.md`](../../services/hub/CLAUDE.md) §2.2).

### Admin credentials (Phase C web-access hardening)

The captive provisioning form requires three fields beyond Wi-Fi + MQTT:

- **Admin username** — default `admin`, can be overridden on the form
- **Admin password** — minimum 8 characters
- **Confirm password** — must match

These are stored as `SHA-256(salt || password)` + 16-byte salt in NVS
namespace `admin`. Plaintext password is never persisted.

After reboot, opening `http://greenhouse.local/` triggers a browser
sign-in popup ("Sign in to Greenhouse Admin").

Auth + rate-limit are installed as **server-global** middleware in
`runOperational()` (`server.addMiddleware(&rateLimit)` then
`server.addMiddleware(&basicAuth)`), so **every** handler is protected —
the `Rest*Routes`, the combined `/api/dashboard` payload, the
`/api/events` SSE source and the `serveStatic("/")` SPA fallback. A
request to any of these without a valid `Authorization` header returns
401; more than 5 attempts / 10 s per IP returns 429. The route classes
no longer attach per-route middleware (that would double the SHA-256
verify).

**To change admin credentials:** hold BOOT for 3 s on reboot → captive
AP appears → fill the same form with a new admin password. Old
credentials stop working immediately.

**Upgrading an existing deployment:** when flashing this branch onto a
board that already has valid Wi-Fi creds but no admin creds, every
`/api/*` request will return 401 (no creds in NVS → verify always
fails). Hold BOOT 3 s on reboot to enter provisioning and set admin
credentials.

---

## [5] Conventions for this firmware

### Build flags

Defined in [`platformio.ini`](platformio.ini):

```
build_flags   = -std=gnu++17 -Wall -Werror -fno-exceptions
build_unflags = -Wextra -Wconversion -Wshadow -fno-rtti
```

`-Wextra` / `-Wconversion` / `-Wshadow` / `-fno-rtti` are dropped because `ESPAsyncWebServer` / `PubSubClient` headers do not compile cleanly under those. This is an honest trade-off documented in the file header; revisit after Phase B.

### Memory rules (no new / no heap in hot path)

- All long-lived objects are `static` inside `runOperational()` (`setup()`-time only).
- `HomeAssistantDiscovery::publishAll()` builds JSON into stack `char[96]` / `char[512]` buffers via `snprintf` — no `std::string` allocations on each MQTT reconnect.
- `MqttClient` internal subscription table is `std::array<Sub, kMaxSubscriptions>` with `char topic[64]` — no heap.
- `MqttTelemetrySink` uses `snprintf` into stack buffers for both topic and payload.

### Single-instance singletons

- `MqttClient::instance_` is a static pointer used to route the PubSubClient C callback to the C++ object. The constructor `assert(instance_ == nullptr)` — only one instance allowed by design.

### NVS namespaces owned by this firmware

| Namespace      | Owner                             | Contents                                         |
|----------------|-----------------------------------|--------------------------------------------------|
| `wifi`         | `NvsWifiCredsStore`               | SSID, password                                   |
| `mqtt`         | `NvsMqttCredsStore`               | host, port, user, password, client_id            |
| `soil_calib`   | `NvsSoilCalibrationStore`         | `raw_dry`, `raw_wet` for `SoilNormalizer`        |
| `zigbee_net`   | `NvsZigbeeNetStore`               | Randomised 8-byte ExtPanId generated on first boot, persisted under key `extpanid`. Sensor-nodes no longer assert ExtPanId during steering, so each coordinator can use a unique value. |
| `admin`        | `NvsAdminCredsStore`              | admin username, salted SHA-256 password hash, 16-byte salt |
| `analytics`    | `NvsAnalyticsConfigStore`         | D-2 only. Keys: `url` (hub ingest URL), `key` (api_key from pairing claim), `period_s` (default 900 = 15 min), `insecure` (TLS bypass for local dev). Empty `url` → analytics path stays dormant. |
| `nodes_alias`  | `NvsNodeAliasStore`               | UTF-8 alias up to 23 bytes per IEEE long address (key = 16-hex IEEE, colon-free). Used by the SPA / REST to display human-friendly node names. |
| `nvs_flags`    | `V1MqttPurge`                     | One-shot upgrade flags. Currently: `mqtt_purge_v1` (bool — set once the v1 retained-topic cleanup has finished). Room for other future one-shot flags. |

The sensor-node owns `zigbee_pair`. Namespaces are disjoint by design (see root [`firmware/CLAUDE.md`](../CLAUDE.md)).

### Pump safety invariants (do not break these)

1. `kPumpMaxRuntimeMs = 20'000` — under no circumstance may the relay stay on longer.
2. Float switch dry → immediate cutoff + `SafetyLocked`.
3. Sensor watchdog stale → `requestOn` is rejected at the gate.
4. On boot, `RelayPump` ctor sets the pin to safe state (LOW for active-high) **before** `pinMode(OUTPUT)`. The board's external pull-down keeps the relay off during the brief high-Z window.

---

## [6] Quick cheat-sheet — where to put code

| I want to add / change…                              | Where                                                                  |
|------------------------------------------------------|------------------------------------------------------------------------|
| New business rule (e.g. "don't water at night")      | `shared/domain/policies/` + test in `test_application/`                |
| New use case (e.g. "auto-calibration")                | `lib/application/src/<area>/` + test in `test_application/`            |
| New REST endpoint                                     | `lib/presentation/src/` — pick the matching `Rest*Routes.cpp` per concern (or add a new module and wire it via the `RestApi` facade) |
| New MQTT entity / sensor channel                      | `TelemetryPublisher` (state topic via `ChannelToTelemetryMapper`) + `HomeAssistantDiscoveryService` (discovery config) |
| New NVS-backed config                                 | `lib/infrastructure/src/persistence/Nvs<Name>Store.cpp` + a port in `shared/domain/src/ports/` |
| Re-tune pins / timeouts                               | `shared/application/AppConfig.hpp` — single source of truth            |
| New Zigbee cluster the coordinator must read          | `ZigbeeBindingTable` (declarative binding) + `ZclSensorMapper` + cluster/attribute IDs in `shared/protocol/src/ChannelAttrTable.hpp` |
| New sensor channel (any node)                         | `shared/protocol/src/ChannelAttrTable.hpp` + sensor-node `ChannelMappings.hpp` + a bit in `gh::domain::SensorKind` |
| Native unit test                                      | `test/test_<name>/` env `coordinator-native`                           |
| Driver / NVS HW test                                  | `test/test_drivers/test_<name>/` env `coordinator-hwtest`              |

---

## [7] Things to NEVER do

- **Never** call `Wire.begin()` from inside a driver — it belongs in the composition root (`setup()`).
- **Never** create FreeRTOS tasks outside `runOperational()` (the deferred-restart helper in `ProvisioningWebServer` is the documented exception — and it is tech-debt to fix in Phase C).
- **Never** use `ESP_ERROR_CHECK(...)` in code that should fail gracefully — it calls `abort()`. Return an `ErrorCode::NetworkDown` (or similar) and log instead. The Zigbee init still has this and is on the cleanup list.
- **Never** introduce `std::string` / `String` / `std::vector` in any function called from a task tick (publish loop, irrigation tick, command handler). Use stack `char[]` + `snprintf` or `std::array`.
- **Never** include `Arduino.h` / `Wire.h` / `WiFi.h` / `FreeRTOS.h` from `lib/application/` or anywhere in `../shared/`. The native test env compiles those folders without a hardware framework.
- **Never** use `GPIO9` for external loads — it is a strapping pin AND the onboard BOOT button. Both jobs are documented in `AppConfig.hpp` as `kBootButtonGpio`.
- **Never** drive the relay GPIO before `pinMode(OUTPUT)` is set — keep the order in `RelayPump` ctor as-is (safe-state first, then `pinMode`).
- **Never** disable the 20 s pump max-runtime cutoff. The cutoff is the last line of defence against a stuck command path. If it ever needs raising, raise the constant in `AppConfig`, do not bypass the check.
- **Never** log MQTT or Wi-Fi passwords via `Serial.print`. Root §9.5 (security MVP trade-offs) — minimum hygiene rules.
