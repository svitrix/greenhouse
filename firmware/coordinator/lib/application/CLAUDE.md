# CLAUDE.md — `lib/application/` (`gh-app-coordinator`)

> Coordinator-specific use cases and orchestrators. Pure C++17 — **no `Arduino.h` / `Wire.h` / `WiFi.h` / FreeRTOS / `millis()`** here, every dependency comes through a port (`I*` interface) from `shared/domain/src/ports/`. Compiles under the `native` host env for unit tests.

## What lives here

Layout — top-level files for cross-cutting glue, plus per-area subdirectories that group related orchestrators:

```
application/src/
├── CoordinatorConfig.hpp        constexpr task/period config
├── AnalyticsUploader.{hpp,cpp}  D-1 telemetry-queue uploader (HTTPS batch)
├── WifiProvisioner.{hpp,cpp}    first-boot Wi-Fi flow
├── irrigation/                  IrrigationService + AutoWaterDecision
├── node/                        NodePruneService
├── telemetry/                   TelemetryPublisher + ChannelToTelemetryMapper + V1MqttPurge
└── zigbee/                      ZigbeeReportRouter
```

| File                                                                                                                    | Purpose                                                                                                                                                                  | Ports it uses                                                                                              |
|-------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------|
| [`CoordinatorConfig.hpp`](src/CoordinatorConfig.hpp)                                                                    | `constexpr` config for stack sizes, task priorities, periods — coordinator-only constants that don't belong in `AppConfig`.                                              | — (header-only)                                                                                            |
| [`WifiProvisioner.{hpp,cpp}`](src/WifiProvisioner.cpp)                                                                  | First-boot flow: check NVS for creds, check BOOT button hold, decide to enter AP/captive portal or proceed to STA.                                                       | `IWifiCredsStore`, `IButton`, `IWifiSta`, `IProvisioningFlagStore`, `IWifiFailCounterStore`, `ILogger`     |
| [`AnalyticsUploader.{hpp,cpp}`](src/AnalyticsUploader.cpp)                                                              | D-1 path: drains a `ITelemetryQueue` (LittleFS) every `flush_period_s`, POSTs batches via `IHttpsClient`. Fed by the `ZigbeeReportRouter` bridge.                       | `ITelemetryQueue`, `IHttpsClient`, `IClock`, `ILogger`                                                     |
| [`irrigation/IrrigationService.{hpp,cpp}`](src/irrigation/IrrigationService.cpp)                                    | Pump-control state machine **with auto + manual modes**. Auto: averages moisture across fresh soil nodes from `INodeRegistry`, gated by `min_fresh_sources` quorum. Enforces `kPumpMaxRuntimeMs` and float-switch dry-run guard. `Off ↔ Running ↔ SafetyLocked`. | `IPump`, `IClock`, `IFloatSwitch`, `INodeRegistry`, `IAutoWaterConfigStore`, `ILogger`                     |
| [`irrigation/AutoWaterDecision.hpp`](src/irrigation/AutoWaterDecision.hpp)                                              | Header-only value object: `should_turn_on` / `reason_code` / `avg_moisture_pct` / `fresh_sources`, returned by the auto-water policy.                                    | — (header-only)                                                                                            |
| [`node/NodePruneService.{hpp,cpp}`](src/node/NodePruneService.cpp)                                                      | Periodic sweep: removes nodes from `INodeRegistry` once they've been offline past the TTL configured via `CoordinatorConfig`.                                            | `INodeRegistry`, `IClock`, `ILogger`                                                                       |
| [`telemetry/TelemetryPublisher.{hpp,cpp}`](src/telemetry/TelemetryPublisher.cpp)                                    | Per-node reconciler: snapshot `INodeRegistry`, diff against last-published state, publish change-only retained / non-retained MQTT topics via `IMqttClient`. Stack-only formatting. | `INodeRegistry`, `IMqttClient`, `IClock`, plus `ChannelToTelemetryMapper`                                  |
| [`telemetry/ChannelToTelemetryMapper.{hpp,cpp}`](src/telemetry/ChannelToTelemetryMapper.cpp)                            | Maps `(SensorKind channel, raw value)` → `(quantity_code, formatted_string)`. Owns the channel → `quantity_code` table and applies `SoilNormalizer` for moisture.        | — (uses `SoilNormalizer` from shared)                                                                      |
| [`telemetry/V1MqttPurge.{hpp,cpp}`](src/telemetry/V1MqttPurge.cpp)                                                      | One-shot retained-cleanup pass: publishes empty-retained payloads to the legacy `greenhouse/<id>/air/*` and `greenhouse/<id>/soil/*` topics, then sets the `mqtt_purge_v1` flag in NVS namespace `nvs_flags` so it never runs again. | `IMqttClient`, NVS `nvs_flags` namespace                                                                    |
| [`zigbee/ZigbeeReportRouter.{hpp,cpp}`](src/zigbee/ZigbeeReportRouter.cpp)                                              | Single fan-out point for incoming Zigbee reports. Parses `ZclAttrEvent` via `ZclSensorMapper` + `ChannelAttrTable`, calls `INodeRegistry.touch / record(channel, value)`, appends to `IHistoryStore`, and bridges to `AnalyticsUploader` via `ITelemetryQueue`. | `INodeRegistry`, `IHistoryStore`, `ITelemetryQueue`, `IClock`, `ILogger`                                   |

## Layering rules (must follow)

- **No hardware headers.** If you find yourself reaching for `Wire.h` / `WiFi.h` / `PubSubClient.h` — that's a missing port. Add the interface to `shared/domain/src/ports/`, then write the adapter in `lib/infrastructure/`.
- **No heap in tick paths.** `IrrigationService::tick()`, `TelemetryPublisher::tick()`, `ZigbeeReportRouter::onReport()` run on every loop / publish cycle. No `std::string`, no `std::vector`, no `new`. Use stack buffers + `snprintf`, or `std::array`.
- **`[[nodiscard]]` everywhere a method returns `ErrorCode`** — callers must handle, not silently ignore.
- **Constructor injection only.** No globals, no static singletons (except `MqttClient::instance_` in infrastructure, which is documented).
- **Tests live in `firmware/coordinator/test/test_<name>/`** under the `coordinator-native` env. New use case → new test, no exceptions (see root [CLAUDE.md](../../../CLAUDE.md) §7).

## Conventions

- File names match class names: `IrrigationService.hpp` declares `class IrrigationService`, period.
- Functions ≤40 lines (root §4). If a method grows past that, extract a private helper.
- Magic numbers go through `CoordinatorConfig` or `AppConfig` (shared), never inline.
- Use `Result<T, ErrorCode>` from `shared/domain/src/util/Result.hpp` for fallible operations that need to return a value; `ErrorCode` alone for void operations.

## Where to add a new use case

1. Identify the ports it needs. If a port is missing — add the `I*` interface to `shared/domain/src/ports/` first.
2. Create `lib/application/src/<Name>.{hpp,cpp}`. Constructor takes the ports by reference (or by value for clocks etc.).
3. Wire it in [`firmware/coordinator/src/main.cpp`](../../src/main.cpp) inside `runOperational()`.
4. Write a native test in `test/test_<name>/test_<name>.cpp` using fakes from [`test/fakes/`](../../test/fakes/).
5. Add to the pre-commit checklist mental model: `pio test -e coordinator-native` must stay green.

## Heads-up — known tech debt

- `IrrigationService::requestOff()` clears `SafetyLocked` as a side effect. A proper `unlock()` (explicit user ack) is on the TODO list — safety-conscious users may want manual re-arm.
- `V1MqttPurge` is intentionally one-shot (gated by NVS `mqtt_purge_v1` flag). Re-running it manually requires deleting the flag via a future REST endpoint or a re-flash with NVS erase. Acceptable for now.
