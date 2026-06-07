# CLAUDE.md — `lib/infrastructure/` (`gh-infra-coordinator`)

> Adapters: hardware drivers, network stacks, NVS persistence, platform glue, FreeRTOS shared types. **Only built under `espressif32`** (declared in `library.json` → `"platforms": ["espressif32"]`). The `coordinator-native` test env skips this whole library — that's why `application/` must not depend on anything here.

## Subsystem map

| Subdirectory                                                                | Local CLAUDE.md                                | What it contains                                                                                                                              |
|-----------------------------------------------------------------------------|------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------|
| [`src/drivers/`](src/drivers/)                                              | [`drivers/CLAUDE.md`](src/drivers/CLAUDE.md)   | GPIO actuators / digital inputs: `RelayPump`, `FakeFloatSwitchAlwaysOk`.                                                                       |
| [`src/network/`](src/network/)                                              | [`network/CLAUDE.md`](src/network/CLAUDE.md)   | Wi-Fi (STA + Soft-AP), MQTT, Zigbee coordinator (`Esp32ZigbeeNetwork`, `ZigbeeCoordinatorAdapter`, `ZigbeeBindingTable`, `ZclSensorMapper`), HTTPS / pairing client, provisioning web server + captive DNS. |
| [`src/persistence/`](src/persistence/)                                      | [`persistence/CLAUDE.md`](src/persistence/CLAUDE.md) | NVS-backed stores: Wi-Fi / MQTT / admin / soil-calibration creds, Zigbee net state, analytics config, auto-water config, node alias, LittleFS telemetry queue. |
| [`src/platform/`](src/platform/)                                            | [`platform/CLAUDE.md`](src/platform/CLAUDE.md) | Clock, GPIO button, Serial logger, Serial telemetry sink.                                                                                       |
| [`src/concurrency/`](src/concurrency/)                                      | [`concurrency/CLAUDE.md`](src/concurrency/CLAUDE.md) | Shared FreeRTOS-queue payload types.                                                                                                            |
| [`src/registry/`](src/registry/)                                            | —                                              | **v2 in-memory state**: `InMemoryNodeRegistry` (IEEE-keyed live node snapshot) + `InMemoryHistoryStore` (per-node, per-channel ring buffer, last 24 h). Implements `INodeRegistry` / `IHistoryStore` ports.|

## Dependencies

From [`library.json`](library.json):

- `gh-domain` — port interfaces (`I*`) that adapters implement.
- `gh-app-shared` — `AppConfig`, `SoilNormalizer`, `JsonTelemetryFormatter`.
- `bblanchon/ArduinoJson@^7.1.0` — JSON parsing for REST handlers + selected HA-Discovery shaping.
- `knolleary/PubSubClient@^2.8` — MQTT client. Wrapped by `MqttClient` adapter.
- ESP-Zigbee-SDK (`esp_zigbee_*`) — pulled in via the Arduino-ESP32 framework, used by `Esp32ZigbeeNetwork` + `ZigbeeCoordinatorAdapter`.
- `ESPAsyncWebServer` / `AsyncTCP` — pulled in via the framework, used by `ProvisioningWebServer` and the `RestApi` facade owning the multi-node REST routes.

## Cross-cutting rules

- **Each adapter implements exactly one port** from `shared/domain/src/ports/`. If you can't name the port, you don't have an adapter — you have a leaky abstraction.
- **Constructor injection of hardware references.** `TwoWire&`, GPIO numbers, addresses come in via the constructor. NEVER call `Wire.begin()` or `pinMode()` from a free function — that's the composition root's job in [`firmware/coordinator/src/main.cpp`](../../src/main.cpp).
- **Safe-state first.** Any actuator (relay, MOSFET gate, LED) must drive its safe state in the constructor BEFORE calling `pinMode(OUTPUT)`. The brief high-Z window during reset is covered by a board-side pull-down. See `RelayPump.cpp` for the canonical pattern.
- **No heap in hot paths.** Everything that runs on every MQTT publish / FreeRTOS tick uses stack `char[]` + `snprintf`, `std::array`, or fixed `Sub`-record tables (see `MqttClient::subs_`).
- **Fixed-width types** (`uint8_t`, `uint32_t`, `int16_t`) at every boundary that touches hardware, the wire protocol, or NVS. Never `int` / `long`.
- **`-fno-exceptions` is mandatory.** All fallible APIs return `ErrorCode` or `std::optional`. No `throw`. `ESP_ERROR_CHECK(...)` is **discouraged** — it calls `abort()`. Prefer `if (err != ESP_OK) return ErrorCode::NetworkDown;` with a log line.

## Where to add a new adapter

1. Add (or confirm) the `I*` port in `shared/domain/src/ports/`.
2. Pick the subdirectory by role (driver / network / persistence / platform / concurrency).
3. Create `<Name>.{hpp,cpp}` in that subdirectory.
4. If the adapter has host-side test fakes — they live in [`firmware/coordinator/test/fakes/`](../../test/fakes/), not here.
5. If it has on-target tests (NVS, Wi-Fi, Zigbee) — put them under `test/test_drivers/test_<name>/` in the `coordinator-hwtest` env.

## Tech-debt callouts inside this layer

- `MqttClient::instance_` is a static singleton (PubSubClient C-callback has no `user_ctx`). The constructor `assert`s `instance_ == nullptr`. Acceptable; documented in [`network/CLAUDE.md`](src/network/CLAUDE.md).
- `xTaskCreate(restartTaskFn, …)` inside an HTTP handler in `ProvisioningWebServer` violates "tasks only in composition root". Documented; refactor when convenient.
- `Esp32ZigbeeNetwork` / `ZigbeeCoordinatorAdapter` still use `ESP_ERROR_CHECK(...)` in `start()` — on init failure the chip silently reboots. Should return `ErrorCode::NetworkDown`. On the cleanup list.
- `WifiStaAdapter::connect()` is a 30-second busy-wait with `delay(100)`. Safe today (called only from `setup()` before `esp_task_wdt_add()`); will break the day a watchdog is registered before WiFi is up.
