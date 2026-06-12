# CLAUDE.md — `lib/presentation/` (`gh-presentation-coordinator`)

> Public-facing surface of the coordinator: MQTT command parsing + HTTP / HA-Discovery surface for the multi-node SPA (see [`docs/design/2026-05-26-coordinator-web-ui-brief.md`](../../../../docs/design/2026-05-26-coordinator-web-ui-brief.md)).

## File map

All REST + HA Discovery + MQTT helpers live directly under [`src/`](src/).

```
presentation/src/
├── MqttCommandRouter.{hpp,cpp}    pump cmd parsing
├── RestHelpers.hpp                shared inline helpers
├── HomeAssistantDiscoveryService.{hpp,cpp}
├── JsonHelpers.{hpp,cpp}
├── NodeViewBuilder.{hpp,cpp}
└── Rest*Routes.{hpp,cpp}          9 REST API route classes (one per concern)
```

### Top-level

| File                                                                                   | Purpose                                                                                                                                                                            |
|----------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| [`MqttCommandRouter.{hpp,cpp}`](src/MqttCommandRouter.cpp)                             | Subscribes to `greenhouse/<dev>/pump/cmd`, parses `"ON"` / `"OFF"` payloads, invokes the bound `on()` / `off()` handlers (set up to call `IrrigationService::requestOn/Off`). The ctor builds the cmd topic once via `std::string` (composition-time allocation, acceptable); the inbound hot path (`onMessage`) does no allocations. |
| [`RestHelpers.hpp`](src/RestHelpers.hpp)                                               | Header-only inline helpers shared by every `Rest*Routes` module: `finishJsonResponse()` (stamp HTTP code + `Cache-Control: no-store` + `send`) and `sendError()` (build the `{ok,error,message}` envelope via ArduinoJson). |

### Multi-node surface

| File                                                                                                                                | Purpose                                                                                                                                                                                                                                                                                                  |
|-------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| [`HomeAssistantDiscoveryService.{hpp,cpp}`](src/HomeAssistantDiscoveryService.cpp)                                           | **One HA "device" per node**, channel-driven entities. Snapshots `INodeRegistry`, builds per-node retained Discovery configs under `homeassistant/<component>/<dev>_<ieee>_<chan>/config` via stack `char[]` + `snprintf`. `reconcile()` is idempotent — safe to call on every MQTT reconnect / tick. |
| [`NodeViewBuilder.{hpp,cpp}`](src/NodeViewBuilder.cpp)                                                                          | Shared helper that materialises a JSON node view (channels + freshness + alias + RSSI etc.) reused by `RestNodesRoutes` and `RestStatusRoutes`. Stack-only formatting via ArduinoJson `JsonObject`.                                                                                                     |
| [`JsonHelpers.{hpp,cpp}`](src/JsonHelpers.cpp)                                                                                  | IEEE long-address → string conversions (colon-free / colon-separated) and other shared JSON-shaping helpers.                                                                                                                                                                                              |
| [`RestNodesRoutes.{hpp,cpp}`](src/RestNodesRoutes.cpp)                                                                          | `GET /api/nodes` — list every node currently in `INodeRegistry` with its channels, RSSI, freshness, alias. Uses `NodeViewBuilder`.                                                                                                                                                                    |
| [`RestNodeAliasRoutes.{hpp,cpp}`](src/RestNodeAliasRoutes.cpp)                                                                  | `PUT /api/nodes/<ieee>/alias` — write a human-friendly node alias to NVS namespace `nodes_alias` (via `INodeAliasStore`). Up to 23 UTF-8 bytes.                                                                                                                                                       |
| [`RestNodeDeleteRoutes.{hpp,cpp}`](src/RestNodeDeleteRoutes.cpp)                                                                | `DELETE /api/nodes/<ieee>` — two-step removal: from `INodeRegistry` + alias store + HA Discovery retained-clear + Zigbee unbinding.                                                                                                                                                                    |
| [`RestHistoryRoutes.{hpp,cpp}`](src/RestHistoryRoutes.cpp)                                                                  | `GET /api/nodes/<ieee>/history?channel=…&hours=1..24` — per-node, per-channel history series from `IHistoryStore`.                                                                                                                                                                                    |
| [`RestPumpRoutes.{hpp,cpp}`](src/RestPumpRoutes.cpp)                                                                        | `POST /api/pump {state: 'ON' \| 'OFF'}` — manual override. Validates input + safety gate (`SafetyLocked` rejection with `details.reason`). Deps: `IrrigationService`.                                                                                                                                  |
| [`RestStatusRoutes.{hpp,cpp}`](src/RestStatusRoutes.cpp)                                                                    | `GET /api/status` — device identity + Wi-Fi + MQTT + Zigbee + pump live state + counts from `INodeRegistry`. Deps: `INodeRegistry`, `IrrigationService`, `ISystemInfo`, `IMqttClient`, `IZigbeeNetwork`.                                                                                              |
| [`RestConfigRoutes.{hpp,cpp}`](src/RestConfigRoutes.cpp)                                                                    | `GET /api/config` snapshot + `POST /api/config` partial-update with `restart_required` echo. Deps: the NVS stores (soil calib / MQTT / Wi-Fi / auto-water / analytics).                                                                                                                            |
| [`RestZigbeeRoutes.{hpp,cpp}`](src/RestZigbeeRoutes.cpp)                                                                    | `POST /api/zigbee/permit_join` — open the Zigbee network for new devices for N seconds. Deps: `IZigbeeNetwork`.                                                                                                                                                                                       |
| [`RestAutoWaterRoutes.{hpp,cpp}`](src/RestAutoWaterRoutes.cpp)                                                              | `GET / POST /api/auto_water` — auto-water policy snapshot + partial update (enabled, threshold, min_fresh_sources, dwell). Deps: `IAutoWaterConfigStore`.                                                                                                                                              |

The `RestApi` facade itself lives in `infrastructure/network/` (it owns `AsyncWebServer`); the route classes register their handlers on it.

## What goes in vs. what goes elsewhere

| Concern                                                          | Lives in                                                                                                                                                                          |
|------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Building per-node state-topic payloads (`greenhouse/<dev>/nodes/<ieee>/...`) | `application/telemetry/TelemetryPublisher` — it's a `INodeRegistry` reconciler, not a discovery thing. Channel formatting goes through `ChannelToTelemetryMapper`.                |
| Parsing inbound MQTT commands                                    | **here** — `MqttCommandRouter`.                                                                                                                                                   |
| HA Discovery payloads (`homeassistant/.../config`)               | **here** — `HomeAssistantDiscoveryService`.                                                                                                                                       |
| HTTP routes (operational UI / SPA on `/api`)                  | **here** — pick the matching `Rest*Routes.cpp` in `presentation/src/` or add a new module and wire it via the `RestApi` facade.                                                                |
| HTTP routes for first-boot provisioning                          | `infrastructure/network/ProvisioningWebServer` — that's a setup-time concern with its own lifecycle.                                                                              |

## Rules

- **No heap in published-on-reconnect paths.** `HomeAssistantDiscoveryService::reconcile()` fires every time MQTT reconnects (every ~5 s in bad-Wi-Fi conditions). One missed allocation → fragmentation → crash in N hours. Stack buffers + `snprintf` only.
- **`unique_id` must be slash-free.** HA refuses `unique_id` containing `/`. There's a regression test under `test_ha_discovery/` — do not remove it.
- **`device_id` (`<dev>`) is hex-only.** Components prepend `greenhouse_` themselves. Do not double-prefix.
- **`<ieee>` is colon-free hex (16 chars).** Use `JsonHelpers::ieeeToColonFreeHex()` everywhere a path / `unique_id` includes the IEEE long address.
- **Handlers are thin.** A MQTT command handler / REST handler must not contain business logic — parse, validate, call into `application/`.
- **Topic / unique_id buffers are ≤ 96 chars** (matches `MqttClient::publish` internal limit). HA Discovery typical: ~80 chars.

## Tests

Native env, no hardware required:

- [`test/test_ha_discovery/`](../../test/test_ha_discovery/) — per-node Discovery configs, retained, topic shape, unique_id slash-free.
- [`test/test_telemetry_publisher/`](../../test/test_telemetry_publisher/) — per-node retained / change-only topic logic, presence-mask reconciliation.
- [`test/test_channel_to_telemetry_mapper/`](../../test/test_channel_to_telemetry_mapper/) — `(channel, raw) → (quantity_code, string)` mapping.
- [`test/test_node_view_builder/`](../../test/test_node_view_builder/) — JSON shape of the node view used by REST.
- [`test/test_json_helpers/`](../../test/test_json_helpers/) — IEEE long-address helpers.
- [`test/test_mqtt_command_router/`](../../test/test_mqtt_command_router/) — `ON` / `OFF` / unknown payload routing, correct subscription topic.

All tests use [`FakeMqttClient`](../../test/fakes/FakeMqttClient.hpp) from `test/fakes/`.

## Web UI contract

The design brief lives at [`docs/design/2026-05-26-coordinator-web-ui-brief.md`](../../../../docs/design/2026-05-26-coordinator-web-ui-brief.md). The SPA in [`firmware/coordinator/web/`](../../web/) consumes the REST routes registered here; every endpoint has a matching `z.object(...)` in [`web/src/api/schemas/schemas.ts`](../../web/src/api/schemas/schemas.ts). When changing a payload, update both sides in the same commit or the SPA's `validate: r => …schema.parse(r)` will reject the response.

- `POST /api/pump` shares the safety state machine with `MqttCommandRouter` — both paths call `IrrigationService::requestOn/Off` and must not duplicate the safety gate.
- Static assets (`index.html`, `assets/*`, `icons.svg`) live on LittleFS, uploaded via `pio run -t uploadfs` from the firmware root's `data/` directory. `partitions.csv` is currently single-factory; dual-OTA is a separate migration.
- **HTTP Basic Auth** from NVS-stored admin creds is enforced on every `/api/*` route by `AsyncAuthenticationMiddleware{AUTH_BASIC}` configured in `RestApi::start()`. The verify callback parses the `Authorization: Basic <base64>` header, SHA-256-hashes the password with the stored salt, and constant-time compares against the stored hash. `AsyncRateLimitMiddleware` limits brute force to 5 req / 10 s per IP. Admin creds are set on the captive provisioning form (see `firmware/coordinator/CLAUDE.md` §4 — admin user/password fields).
