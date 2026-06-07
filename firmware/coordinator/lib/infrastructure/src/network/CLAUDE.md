# CLAUDE.md — `infrastructure/src/network/`

> All wireless and IP-stack adapters: Wi-Fi (STA + Soft-AP), MQTT, Zigbee coordinator (multi-node), analytics HTTPS + pairing client, captive-DNS, and the provisioning HTTP server. This is the **largest and trickiest** subsystem — read the file table before editing. Reflects the **Phase D multi-node** pipeline (`v2.0.0-multinode`); the v1 single-node `SensorCache` / `MqttTelemetrySink` / stateless `void(*)` Zigbee callbacks are **gone** — do not reintroduce them.

## File map

### Wi-Fi

| File                                                            | Implements              | Notes                                                                                                              |
|-----------------------------------------------------------------|-------------------------|--------------------------------------------------------------------------------------------------------------------|
| [`WifiStaAdapter.{hpp,cpp}`](WifiStaAdapter.cpp)                | `gh::domain::IWifiSta`  | STA connect + runtime link recovery. `begin()` (call once at boot) registers a `WiFi.onEvent(ARDUINO_EVENT_WIFI_STA_DISCONNECTED)` handler that maps the ESP disconnect reason → `ConnectError` (persisted via `ILastConnectErrorStore`) and, **after the first successful association**, nudges `WiFi.reconnect()` — except on `AuthFail`/`SsidNotFound` (retrying there just spams). Also sets `setAutoReconnect(true)`. `connect()` disables modem power-save (`setSleep(false)` — see [Wi-Fi ↔ Zigbee coexistence](#wi-fi--zigbee-coexistence-single-radio-esp32-c6)) and is still a **busy-wait** with `delay(100)` up to `timeout_ms` — safe only because called from `setup()` before any watchdog enrolment (tech debt: move fully event-driven). On timeout calls `WiFi.disconnect(false)` (soft) so callers can retry without re-issuing `WiFi.mode(STA)`. The `connected_once_` gate is `std::atomic<bool>` (written from the connect/setup context, read from the WiFi event task). A coarser belt-and-suspenders supervisor also lives in `main.cpp` `coordinator_task` (reconnect after ~60 s offline). |
| [`WifiSoftApAdapter.{hpp,cpp}`](WifiSoftApAdapter.cpp)          | (Soft-AP helper, no port) | Brings up `Greenhouse-Setup-XXXX` AP **with WPA2 + per-device passphrase** (caller derives from MAC, logs it via `ILogger::info` at boot). Fixed gateway `192.168.4.1`. Passing `nullptr`/empty for passphrase falls back to an open AP (escape hatch, NOT recommended). `softApIP()` returns the current AP gateway. |

### MQTT

| File                                                            | Implements                  | Notes                                                                                                          |
|-----------------------------------------------------------------|-----------------------------|----------------------------------------------------------------------------------------------------------------|
| [`MqttClient.{hpp,cpp}`](MqttClient.cpp)                        | `gh::domain::IMqttClient`   | Wraps `bertmelis/espMqttClient` (async, runs on AsyncTCP task). Ctor configures: `setServer(host, port)`, optional `setCredentials(user, pass)` (only if `creds.user` non-empty), `setClientId`, `setCleanSession(true)`, `setKeepAlive(30 s)`. Subscription table is `std::array<Sub, kMaxSubscriptions=4>` with `char topic[64]`, no heap. Reconnect is driven by the library; `loop()` is a no-op kept only for the `IMqttClient` port. `onConnect` lambda calls `resubscribeAll()` (clean sessions, so we always re-subscribe). `connect()` issues the request and then polls `connected()` for ≤ 5 s so callers see a consistent state. |

### Zigbee (multi-node)

| File                                                            | Implements                          | Notes                                                                                                              |
|-----------------------------------------------------------------|-------------------------------------|--------------------------------------------------------------------------------------------------------------------|
| [`ZigbeeCoordinatorAdapter.{hpp,cpp}`](ZigbeeCoordinatorAdapter.cpp) | `gh::domain::IZigbeeCoordinator` | Forms the Zigbee network as **Coordinator** (`ESP_ZB_DEVICE_TYPE_COORDINATOR`), launches the `zb_task` main loop (8 KB stack — **bytes**, ESP-IDF deviation from vanilla FreeRTOS words), opens the initial permit-join on formation. The static `zb_action_handler` decodes inbound ZCL Report-Attributes and dispatches **per-attribute** into the `gh::domain::IZigbeeReportSink` set via `setReportSink`: EP1 Basic `0xF001` → `onPresenceFrame(mask)`, `0xF002` → `onPresenceFrame(proto_version)`, everything else via `ZclSensorMapper::decode` → `onChannelSample`. No per-source staging — the application-side `ZigbeeReportRouter` reassembles. ZDO `DEVICE_ANNCE`/`LEAVE_INDICATION` → `onDeviceAnnounced`/`onDeviceLeft`. **Self-heal:** on first sight of a `short_addr` each session the handler resolves its IEEE from the stack address map (`esp_zb_ieee_address_by_short`) and re-seeds the binding, so reports still resolve after a warm reboot when an already-joined sleepy node never re-announces. Hardened: custom TC link key, per-coordinator ExtPanId, channel-mask pin (ch 25), min-LQI join gate — see `secrets.hpp` / `shared/protocol/src/ZigbeeNetwork.hpp`. Stack-API mutations (`writeReportPeriod`, `openPermitJoin`, `requestLeave`) take `esp_zb_lock_acquire(500 ms)`. All `start()` failure paths return `ErrorCode::NetworkDown` (the prior `ESP_ERROR_CHECK(...)` → silent `abort()` was removed). |
| [`Esp32ZigbeeNetwork.{hpp,cpp}`](Esp32ZigbeeNetwork.cpp)        | `gh::domain::IZigbeeNetwork`         | Thin facade the **application/presentation** layers call for on-air admin commands: `permitJoin(duration_s)` (clamped 1..254) and `requestLeave(NodeId)`. Delegates to `IZigbeeCoordinator` for the on-air command and uses `ZigbeeBindingTable::shortAddrFor` to map `NodeId` (IEEE) → `short_addr`. |
| [`ZigbeeBindingTable.{hpp,cpp}`](ZigbeeBindingTable.cpp)        | (pure C++ map, no port)             | In-RAM `etl::flat_map<uint16_t short_addr, NodeId, 8>` — **the only `short_addr ↔ IEEE` directory** the application sees. Populated on `onDeviceAnnounced` (real ZDO annce **or** the adapter's self-heal seed), cleared on `onDeviceLeft`. **RAM-only** (not persisted) — survives across reboot only via the self-heal path above. On a re-announce with a known IEEE it evicts the stale `short_addr`; on overflow it drops the oldest (capacity 8 = `gh::domain::kMaxRegisteredNodes`, so within design limits it never overflows). Pure (only `<etl/...>` + `NodeId`) → host-compilable, hence the application may depend on it directly. |
| [`ZclSensorMapper.{hpp,cpp}`](ZclSensorMapper.cpp)             | (pure decode helper, no port)       | `decode(endpoint, cluster, attr, raw, len)` → `optional<{SensorKind, Quantity, value_si}>`. Looks up `(ep, cluster, attr)` in `shared/protocol/src/ChannelAttrTable.hpp`, parses the ZCL scalar (with `len` bounds checks), and converts to SI (e.g. centi-°C → °C). Returns `nullopt` for unknown/truncated attributes (silently dropped by the handler). |

### Analytics HTTPS + pairing (D-1 / D-2)

| File                                                            | Implements                  | Notes                                                                                                          |
|-----------------------------------------------------------------|-----------------------------|----------------------------------------------------------------------------------------------------------------|
| [`EspHttpsClient.{hpp,cpp}`](EspHttpsClient.cpp)               | `gh::domain::IHttpClient`    | D-1 analytics path: HTTPS `POST` of batched telemetry to the FastAPI hub. Drives `WiFiClientSecure` (optional `insecure` TLS bypass for local dev). Consumed by `application/AnalyticsUploader`. |
| [`EspPairingClient.{hpp,cpp}`](EspPairingClient.cpp)           | `gh::domain::IPairingClient` | D-2 Hue-style pairing: builds the `/api/pairing/claim` URL + JSON body via stack buffers, posts via `EspHttpsClient`, parses `{api_key}` with `StaticJsonDocument<256>`. Driven synchronously inside the captive-portal form POST — see coordinator `CLAUDE.md` § "Analytics + pairing flow". |

### Provisioning (captive portal)

| File                                                            | Purpose                                                                                                                                       |
|-----------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------|
| [`ProvisioningWebServer.{hpp,cpp}`](ProvisioningWebServer.cpp)  | `ESPAsyncWebServer` wrapper. Single public method `start()` registers all routes and calls `server_.begin()`. Routes — see [Routes table](#routes-served-by-provisioningwebserver) below. **Known tech debt:** `xTaskCreate(restartTaskFn, ...)` inside the `POST /save` handler defers `esp_restart()` until after the response. Violates "tasks only in composition root"; move to a cmd-queue when convenient. |
| [`CaptiveDnsServer.{hpp,cpp}`](CaptiveDnsServer.cpp)            | Thin wrapper around Arduino-ESP32's `DNSServer`. Resolves every query to the AP-IP passed into `start(ap_ip)` so iOS / Android trigger the captive-portal flow on AP join. `processNext()` is called from a dedicated FreeRTOS task in `main.cpp`. |
| [`provisioning_html.hpp`](provisioning_html.hpp)                | The provisioning page as a single `PROGMEM` C string. Served as a **fallback** by `GET /` only when LittleFS has neither `/index.html.gz` nor `/index.html` (i.e. first install before the SPA image is uploaded). Update via raw-string-literal edit. |

## Wi-Fi ↔ Zigbee coexistence (single radio, ESP32-C6)

**Both run concurrently today and reception works — but Espressif officially classifies our exact configuration as "unstable".** This is a hardware constraint, not a code bug.

The ESP32-C6 has **one shared 2.4 GHz RF path** for Wi-Fi / BLE / 802.15.4, time-division-multiplexed. Per the [ESP-IDF RF Coexistence guide (ESP32-C6)](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-guides/coexist.html):

| Wi-Fi STA | 802.15.4 **Router** | 802.15.4 **End Device** |
|---|---|---|
| Connected | **C1 — supported but unstable** | Y — stable |

- Our role is **Coordinator** (rx-on-when-idle FFD) → behaves like the **Router** column → **C1**. There is no "Coordinator" row; the table only lists Scan / Router / End Device.
- *"normal 802.15.4 receive operations are assigned the lowest priority"* → heavy Wi-Fi/BLE traffic raises Zigbee packet loss.
- Coexistence is **automatic**: *"For most coexistence cases, ESP32-C6 will switch the coexistence status automatically without calling API."* → **do not** call `esp_coex_*` (only BLE-MESH needs manual calls).
- It must be compiled in via `CONFIG_ESP_COEX_SW_COEXIST_ENABLE` — **already `=y`** in the precompiled `framework-arduinoespressif32-libs/esp32c6/sdkconfig`. Nothing to do in `platformio.ini`.
- For a production Wi-Fi-based Zigbee gateway Espressif **recommends a dual-SoC** design (e.g. ESP32-C6/H2 RCP + a host SoC) with separate antennas. Out of scope for this DIY build.

**Why it's tolerable for us:** sleepy nodes report rarely (~300 s), MQTT/analytics are batched, Zigbee is pinned to ch 25 (clear of Wi-Fi 1/6/11). Low contention + the self-heal/retry paths above absorb the occasional dropped frame. **If you change anything that raises Wi-Fi airtime or makes Zigbee latency-sensitive, re-measure.** One knob worth A/B-testing against Zigbee RX loss: `WIFI_PS_MIN_MODEM` vs the current `WIFI_PS_NONE` — modem-sleep frees radio windows for 802.15.4 RX, but a prior A/B favoured `NONE` for Wi-Fi *association* stability (see the comment in `WifiStaAdapter.cpp`). The two goals trade off; the choice is config, not architecture.

## Routes served by `ProvisioningWebServer`

| Direction        | Path             | Handler shape                                                                                                                            |
|------------------|------------------|------------------------------------------------------------------------------------------------------------------------------------------|
| `GET`            | `/`              | Serves `/index.html.gz` (gzip-encoded) → `/index.html` → inline `kProvisioningHtml`, in that order. `Cache-Control: no-store` on each.   |
| `GET`            | `/scan`          | **Async.** Triggers `WiFi.scanNetworks(true, ...)` if no scan in progress or cache age > 30 s. Returns `202 {status:"scanning", retry_after_ms:500}` while running, `200 {networks:[…]}` once complete. |
| `GET` (static)   | `/assets/*`      | From LittleFS, `Cache-Control: public, max-age=31536000, immutable`.                                                                     |
| `GET` (static)   | `/icons.svg`     | From LittleFS, `Cache-Control: public, max-age=86400`.                                                                                   |
| `GET`            | `/api/status`    | Provisioning-mode status JSON: `device_id`, `uptime_s`, `firmware_version`, fixed `ip:"192.168.4.1"`, `mode:"provisioning"`. STA / MQTT fields are intentionally absent here — the operational REST API serves the rich `/api/status`. |
| `POST`           | `/save`          | Saves Wi-Fi + MQTT creds (required), soil calibration (optional), admin creds, and optional analytics URL + pairing code (runs the D-2 claim inline). Clears the `forced_provisioning` flag, responds `200 {status:"saved"}`, then a deferred 3-s task fires `ESP.restart()`. |
| `GET`            | `/status`        | Legacy mini-payload `{mode:"provisioning",ip:"192.168.4.1"}` kept for backward compat with the earliest HTML.                            |
| `404` fallback   | (anything else)  | `redirect("http://192.168.4.1/")` so captive-portal sniffers (Apple `/hotspot-detect.html`, Android `/generate_204`) land on the form.   |

## Topic / endpoint cheat-sheet (multi-node)

`<dev>` = coordinator 6-hex device id; `<ieee>` = node 16-hex IEEE (colon-free); `<quantity_code>` ∈ `temp_c | humidity_pct | moisture_pct | soil_temp_c | pct | voltage_v`.

| Direction    | Topic / URL                                                          | Owner                                                  |
|--------------|---------------------------------------------------------------------|--------------------------------------------------------|
| MQTT pub     | `greenhouse/<dev>/nodes/<ieee>/{online,present_mask,proto_version,rssi_dbm,<quantity_code>}` | `application/telemetry/TelemetryPublisher`             |
| MQTT pub     | `greenhouse/<dev>/pump/state`                                       | `application/telemetry/TelemetryPublisher`             |
| MQTT pub     | `homeassistant/{sensor,switch,...}/<dev>_<ieee>_<chan>/config`      | `presentation/HomeAssistantDiscoveryService`           |
| MQTT sub     | `greenhouse/<dev>/pump/cmd`                                         | `presentation/MqttCommandRouter`                       |
| HTTPS POST   | `<hub_url>/ingest`, `<hub_url>/api/pairing/claim`                   | `EspHttpsClient` / `EspPairingClient`                  |
| HTTP (prov.) | see [Routes table](#routes-served-by-provisioningwebserver) above   | `ProvisioningWebServer`                                |
| HTTP (oper.) | `GET /api/status,nodes,history`, `POST /api/pump`, etc.            | `presentation/Rest*Routes` via the `RestApi` facade    |
| Zigbee EP1   | clusters Basic `0x0000` (`0xF001` present_mask / `0xF002` proto_version), Power `0x0001` (battery `0x0021` / voltage `0x0020`), Temp `0x0402`, Humidity `0x0405`, Soil moisture `0x0408` | `ZigbeeCoordinatorAdapter` → `ZclSensorMapper`         |
| Zigbee EP2   | cluster Temperature `0x0402` (Chirp soil temperature)               | `ZigbeeCoordinatorAdapter` → `ZclSensorMapper`         |

## Rules specific to this directory

- **`MqttClient` routes inbound messages via a captured `this`** in the `espMqttClient::onMessage` lambda. The previous PubSubClient-era static singleton is gone — multiple instances are technically allowed (today only one is wired in `main.cpp`).
- **MQTT subscription matching is exact-string only.** `MqttClient::dispatchMessage` walks the `subs_` table comparing `topic_sv == s.topic`. MQTT wildcards (`+`, `#`) are accepted at the broker but the local dispatch will not deliver them — keep subscriptions concrete.
- **MQTT topic buffer is stack-only.** `MqttClient::publish` copies the topic into `char topic_buf[96]` — exceed 95 chars and you'll get back `ErrorCode::Unknown`. Payload size is **uncapped** (espMqttClient streams arbitrary sizes; the old 256 B PubSubClient buffer limit is gone).
- **Zigbee inbound goes through `IZigbeeReportSink`, NOT `void(*)` callbacks.** The adapter holds a single `static IZigbeeReportSink* s_sink` (set via `setReportSink`); the application-side `ZigbeeReportRouter` implements the sink. There are no per-quantity registration calls and no `void* ctx` — that v1 API is gone.
- **Set the report sink BEFORE `start()`.** `start()` launches `zb_task`, which can dispatch frames immediately; a sink wired afterwards races the first reports (the handler drops frames while `s_sink == nullptr`).
- **Zigbee callbacks fire on the `zb_task` context.** State shared with other tasks (the `InMemoryNodeRegistry` / `InMemoryHistoryStore` the router writes) is lock-free **SPSC** — `zb_task` is the sole producer; `coordinator_task` / `telemetry_task` only read via `snapshot*()`. Do not add a second producer without a lock or queue.
- **The binding table is RAM-only.** After a warm reboot it is empty until the self-heal seed (or a fresh `DEVICE_ANNCE`) repopulates it; reports with an unresolved `short_addr` are dropped by the router. If you raise node count past `kMaxRegisteredNodes` (8), grow the `etl::flat_map` capacity here **and** the registry/history capacities together.
- **`/api/status` here is the provisioning slim version.** Do not add STA/MQTT fields — the device is AP-only in this mode; the operational `lib/presentation/RestStatusRoutes.cpp` serves the rich version.
- **`/save` validates soil calibration only when at least one of `soil_dry`/`soil_wet` is present.** Both omitted → skip; only one → 400; both → `valid()` check (`raw_dry < raw_wet`).
- **`CaptiveDnsServer::processNext()` must be polled.** Called from the dedicated `dns_task` (2 KB) in `main.cpp`; never call `DNSServer::processNextRequest()` from a web handler.

## Adding a new endpoint / handler / cluster

| Adding…                                       | Where                                                                                                                                                                                              |
|-----------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| A new MQTT publish topic                      | `application/telemetry/TelemetryPublisher` for state topics, `presentation/HomeAssistantDiscoveryService` for discovery configs. Don't touch `MqttClient` itself.                                  |
| A new MQTT subscription / command             | `presentation/MqttCommandRouter` (subscribe + parse). The handler stays thin, delegating to `application/`.                                                                                        |
| A new HTTP route (provisioning)               | `ProvisioningWebServer::start()` (this directory). Keep handlers thin; delegate to the injected stores (`IWifiCredsStore` / `IMqttCredsStore` / `ISoilCalibrationStore` / …).                       |
| A new HTTP route (operational)                | Don't add to `ProvisioningWebServer`. Pick (or create) a `Rest*Routes` module in `lib/presentation/` and wire it via the `RestApi` facade.                                                          |
| A new Zigbee cluster / sensor channel         | Add the `(endpoint, cluster, attr)` row + `SensorKind`/`Quantity` to `shared/protocol/src/ChannelAttrTable.hpp` — `ZclSensorMapper` then decodes it automatically. No `zb_action_handler` edit needed unless it's a new Basic-cluster control attribute (like `0xF001`/`0xF002`). |
| A new on-air admin command (leave/permit/etc.)| Extend `IZigbeeCoordinator` + `ZigbeeCoordinatorAdapter`, expose it through `IZigbeeNetwork`/`Esp32ZigbeeNetwork` if the application needs it.                                                        |

## Tests

| Test                                                                                                | Env                | What it covers                                                                                                                                                       |
|-----------------------------------------------------------------------------------------------------|--------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| [`test/test_drivers/test_mqtt_client/test_main.cpp`](../../../../test/test_drivers/test_mqtt_client/test_main.cpp) | `coordinator-hwtest` | Round-trip against a real broker via Wi-Fi: connect, short-payload publish/subscribe, **512-byte payload regression** (proves the old PubSubClient 256 B cap is gone), exact-match dispatch (no wildcard delivery). Requires `test_creds.hpp` (gitignored) — see the co-located `.example`. Run: `pio test -e coordinator-hwtest -f 'test_drivers/test_mqtt_client' -d firmware/coordinator`. |

`ZclSensorMapper` and `ZigbeeBindingTable` are pure C++ and **could** be host-tested (no hardware) — there are no dedicated tests yet, but `test_zcl_sensor_mapper` (native) exercises the mapper's decode table. `WifiStaAdapter`, `WifiSoftApAdapter`, `CaptiveDnsServer`, `ProvisioningWebServer`, `ZigbeeCoordinatorAdapter`, `EspHttpsClient`, `EspPairingClient` are **not** auto-tested (need a broker / sensor-node / test Wi-Fi / real radio). Their callers are covered via fakes (`FakeMqttClient` / `FakeWifiSta`).

## Security caveats (root §9.5)

- Wi-Fi password and MQTT password are stored in **unencrypted NVS**. Documented MVP trade-off.
- `secrets.hpp` (Zigbee TC link key + ExtPanId) is compiled into the firmware blob. `.gitignore`d; only `secrets.hpp.example` is committed.
- The provisioning Soft-AP uses **WPA2** with a per-device passphrase derived from the MAC (`gh-XXXXXXXX`), logged via `ILogger::info` at boot. Defeats passive sniffing of the form POST that carries Wi-Fi + MQTT passwords. The provisioning passphrase is the **only password** legitimately printed to Serial.
- The D-2 pairing `api_key` returned by the hub is persisted to NVS `analytics`; the 6-digit claim code is transient (never stored).
- **Never** log Wi-Fi / MQTT / broker password via `Serial.print` — not even in debug builds.
