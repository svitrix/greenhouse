# CLAUDE.md — `firmware/coordinator/lib/`

> Local libraries for the coordinator firmware, registered as PlatformIO `lib_deps`. Clean-architecture layering is in root [`CLAUDE.md`](../../../CLAUDE.md) §1. Hardware-level context is in [`../CLAUDE.md`](../CLAUDE.md).

Every subdirectory here is a self-contained PlatformIO library with its own `library.json`. Each has a local `CLAUDE.md` — read it before touching files inside that subtree.

## Layer map

```
firmware/coordinator/lib/
├── application/      gh-app-coordinator         use cases / orchestrators (irrigation/, node/, telemetry/, zigbee/)
├── infrastructure/   gh-infra-coordinator       adapters (drivers, network, persistence, platform, concurrency, registry)
└── presentation/     gh-presentation-coordinator MQTT command router + HA Discovery + 9 REST API route classes
```

| Layer | `library.json` name           | Platforms       | May depend on                           |
|-------|-------------------------------|-----------------|-----------------------------------------|
| `application/`    | `gh-app-coordinator`           | all (host-testable) | `gh-domain`, `gh-app-shared`            |
| `infrastructure/` | `gh-infra-coordinator`         | **`espressif32` only** | `gh-domain`, `gh-app-shared`, ArduinoJson, PubSubClient, esp-zigbee-sdk, ESPAsyncWebServer |
| `presentation/`   | `gh-presentation-coordinator`  | all (host-testable) | `gh-domain`, `gh-app-shared`, `gh-app-coordinator` |

> **Layering rule:** dependencies point inward only. `application/` MUST NOT include `Arduino.h` / `Wire.h` / `WiFi.h` / `FreeRTOS.h` — those folders are compiled in the `native` test env without any hardware framework. Only `infrastructure/` may pull in framework headers.

## Where to add new code

| You want to add…                                          | Layer / subdirectory                                                            |
|-----------------------------------------------------------|----------------------------------------------------------------------------------|
| A new business orchestrator (uses ports, no hardware)     | `application/src/<area>/` (`irrigation/`, `node/`, `telemetry/`, `zigbee/`)      |
| A new hardware driver (I²C / GPIO / relay)                | `infrastructure/src/drivers/`                                                    |
| A new networking adapter (Wi-Fi / MQTT / Zigbee / HTTP)   | `infrastructure/src/network/`                                                    |
| A new NVS-backed config store                             | `infrastructure/src/persistence/`                                                |
| A new in-memory registry / history store                  | `infrastructure/src/registry/`                                                   |
| A clock / logger / button / serial-port adapter           | `infrastructure/src/platform/`                                                   |
| A FreeRTOS queue / shared concurrency type                | `infrastructure/src/concurrency/`                                                |
| A new MQTT topic / HA Discovery payload                   | `presentation/src/` (state via `TelemetryPublisher`, discovery via `HomeAssistantDiscoveryService`) |
| A new REST endpoint                                       | `presentation/src/` (matching `Rest*Routes.cpp` or new module wired via the `RestApi` facade) |
| A port interface (used by both firmwares)                 | `../../shared/domain/src/ports/`  (NOT here)                                  |
| A pure value object / entity                              | `../../shared/domain/src/entities/`                                           |

## Quick links

- [`application/CLAUDE.md`](application/CLAUDE.md) — services that orchestrate the dependency graph at runtime.
- [`infrastructure/CLAUDE.md`](infrastructure/CLAUDE.md) — adapters and their subsystem map.
- [`presentation/CLAUDE.md`](presentation/CLAUDE.md) — MQTT side of the public interface.
