# CLAUDE.md — Greenhouse ESP32-C6 firmware

> This file **complements** the global `~/Work/CLAUDE.md` (hard embedded rules: memory / ISR / RTOS / types / runtime restrictions). Global rules take priority — this file covers only the architecture and code style specific to this project.
>
> **Hardware / board / pinout details live one level down**: shared technical stack and ESP32-C6 chip reference are in [`firmware/CLAUDE.md`](firmware/CLAUDE.md); per-board specifics in [`firmware/coordinator/CLAUDE.md`](firmware/coordinator/CLAUDE.md) (DevKitM-1) and [`firmware/sensor-node/CLAUDE.md`](firmware/sensor-node/CLAUDE.md) (SuperMini). BOM and wiring rationale — [`COMPONENTS.md`](COMPONENTS.md).
>
> **Cloud-side analytics + Hue-style admin hub**: the Python/FastAPI hub service that coordinators POST to (long-term storage + admin REST + device profile catalog + pairing flow) lives in [`services/hub/`](services/hub/) — its own world (Python, Docker, Postgres+Timescale), separate from the embedded rules below. See [`services/hub/CLAUDE.md`](services/hub/CLAUDE.md) before touching anything there. The React/TS admin UI lives in [`services/dashboard/`](services/dashboard/).

---

## [1] CLEAN ARCHITECTURE — LAYERS & DEPENDENCIES

Dependencies point **inward only**. Inner layers know nothing about outer ones.

```
┌─────────────────────────────────────────────────────────────┐
│ presentation/   REST routes · MQTT topics · HA Discovery     │  ─┐
├─────────────────────────────────────────────────────────────┤   │
│ infrastructure/ ChirpDriver · AM2315CDriver · RelayPump ·    │   │ depend
│                 WifiAdapter · MqttClient · NvsConfigStore    │   │   on
├─────────────────────────────────────────────────────────────┤   │ inner
│ application/    IrrigationService · TelemetryPublisher       │   │ layers
├─────────────────────────────────────────────────────────────┤   │
│ domain/         SoilSample · AirSample · WateringPolicy ·    │  ─┘
│                 ISoilSensor · IPump · IClock · ILogger       │
└─────────────────────────────────────────────────────────────┘
                                ▲
                         main.cpp (composition root)
                         builds the dependency graph
```

### What belongs in each layer

| Layer | Contents | May include |
|---|---|---|
| `domain/`         | Entities, value objects, business rules, **port interfaces** | only STL-typed utilities (`std::optional`, `etl::*`); **no `Arduino.h`, `Wire.h`, FreeRTOS** |
| `application/`    | Use cases / services, orchestrating domain operations via ports | `domain/`; no hardware |
| `infrastructure/` | Port adapters: I²C drivers, GPIO / relay, Wi-Fi, MQTT, NVS, FS | `domain/` (for interfaces); `application/` not allowed; Arduino / IDF allowed |
| `presentation/`   | REST endpoints, MQTT discovery payloads, parsing of inbound commands | `application/` (calls use cases), `domain/` (types) |
| `main.cpp`        | Composition root: `setup()` instantiates adapters and wires them into services | anything |

**Rule:** if a function in `domain/` or `application/` uses `digitalWrite`, `delay`, `Wire`, `WiFi`, `millis` — that's an architecture bug. Replace it with a port.

---

## [2] DIRECTORY STRUCTURE (PlatformIO)

```
green-house-esp32/
├── firmware/
│   ├── shared/             (C++17 libs shared by both firmwares)
│   │   ├── domain/         (gh-domain — entities, ports, policies, errors, util)
│   │   ├── application/    (gh-app-shared — SoilNormalizer, JsonTelemetryFormatter, AppConfig)
│   │   └── protocol/       (gh-protocol — Zigbee ZCL IDs, placeholder in Phase A)
│   ├── coordinator/
│   │   ├── platformio.ini  (envs: coordinator, coordinator-native, coordinator-hwtest)
│   │   ├── partitions.csv
│   │   ├── src/main.cpp
│   │   ├── lib/
│   │   │   ├── application/        (gh-app-coordinator: IrrigationService, TelemetryPublisher, WifiProvisioner)
│   │   │   ├── infrastructure/     (gh-infra-coordinator: drivers, persistence, platform, network, concurrency)
│   │   │   └── presentation/       (Phase C: RestApi, MqttRoutes, HomeAssistantDiscovery)
│   │   └── test/
│   │       ├── fakes/
│   │       ├── test_<name>/        (native unit tests)
│   │       └── test_drivers/       (hwtest)
│   └── sensor-node/
│       ├── platformio.ini  (envs: sensor-node, sensor-node-native)
│       ├── partitions.csv
│       └── src/main.cpp            (Phase A: WS2812 blink stub)
├── services/
│   ├── hub/                (Python/FastAPI: ingest + admin REST + Postgres/Timescale)
│   └── dashboard/          (React/TS admin UI)
└── archive/legacy-monolith/        (previous root-level src/lib/test/...)
```

**One class — one pair of files** (`*.hpp` + `*.cpp`). No 1000-line dump files.

---

## [3] PORTS & ADAPTERS — TEMPLATE

A port is an abstract interface in `domain/`. An adapter is a concrete implementation in `infrastructure/`.

```cpp
// lib/domain/ports/ISoilSensor.hpp
#pragma once
#include <cstdint>
#include <optional>
#include "domain/entities/SoilSample.hpp"

namespace gh::domain {
class ISoilSensor {
public:
    virtual ~ISoilSensor() = default;
    [[nodiscard]] virtual std::optional<SoilSample> read() noexcept = 0;
};
}
```

```cpp
// lib/infrastructure/drivers/ChirpSoilSensor.hpp
#pragma once
#include <Wire.h>
#include "domain/ports/ISoilSensor.hpp"

namespace gh::infra {
class ChirpSoilSensor final : public gh::domain::ISoilSensor {
public:
    explicit ChirpSoilSensor(TwoWire& bus, uint8_t address = 0x20) noexcept;
    [[nodiscard]] std::optional<gh::domain::SoilSample> read() noexcept override;
private:
    TwoWire& bus_;
    uint8_t  address_;
};
}
```

```cpp
// src/main.cpp — composition root
void setup() {
    static gh::infra::ChirpSoilSensor   soil{Wire, 0x20};
    static gh::infra::AM2315CSensor     air{Wire, 0x38};
    static gh::infra::RelayPump         pump{/*gpio*/10, /*max_runtime_ms*/20'000};
    static gh::infra::ArduinoClock      clock{};
    static gh::infra::SerialLogger      log{};

    static gh::app::IrrigationService irrigation{soil, pump, clock, log,
                                                  gh::app::IrrigationConfig{/*...*/}};
    // …
}
```

**An adapter's constructor takes its dependencies explicitly** (`TwoWire&`, GPIO number, address). No global `Wire.begin()` inside the driver — that happens in the composition root.

---

## [4] CLEAN CODE — MANDATORY

### Naming
- **PascalCase** — types, classes, enum class · **camelCase** — functions, variables · **SCREAMING_SNAKE** — `constexpr` constants · **m_** or trailing `_` for private fields (pick one and stick with it) · `I` prefix for interfaces (`ISoilSensor`).
- The name must explain **why**, not how: `maxPumpRuntimeMs`, not `t1`. Units in the name are mandatory: `*_ms`, `*_bytes`, `*_hz`, `*_pct`.

### Functions
- ≤ 40 lines (rule from the global CLAUDE). If longer — extract private helpers.
- One level of abstraction per function. A loop + a business check + I/O inside one function — refactor it.
- Return the **result**, not a code via `out`-parameters. Use `std::optional<T>` or `Result<T, Error>` (see [§5]).
- Pure functions by default. Side effects only in adapters or explicit "procedures".

### Classes
- Single Responsibility. If a class name contains "And" — it's two classes.
- Constructor injection of all dependencies. No `new` / `malloc` inside (see global rule [1]). No globals or singletons in `domain/application`.
- Class state minimal. If a field is only used in one method — it's a local variable.

### Magic numbers & constants
- All "bare" numbers → `constexpr` or `enum class`:
  ```cpp
  constexpr uint32_t kPumpMaxRuntimeMs       = 20'000;
  constexpr uint16_t kI2cBusFrequencyHz      = 100'000;
  constexpr uint8_t  kChirpDefaultAddress    = 0x20;
  ```
- Configurable values go through `IConfigStore` / `AppConfig`, not hardcoded.

### Comments
- By default — **do not write any**. Names and types carry the meaning.
- Write only when WHY is non-obvious: hardware quirk, datasheet reference, regression guard. Do not describe WHAT.
- Do not leave commented-out code.

---

## [5] ERROR HANDLING (without exceptions)

`-fno-exceptions` is mandatory (global [13]). Therefore:

- I/O functions return `std::optional<T>` (value present / not) **or** `Result<T, ErrorCode>` — status + value.
- A single `enum class ErrorCode` per project (`domain/errors/ErrorCode.hpp`).
- `[[nodiscard]]` is mandatory on every function that returns an error code or a resource.
- In `presentation/`, errors are mapped to HTTP codes and MQTT payloads; they do not bubble up.
- `assert` / `abort` are forbidden at runtime — use `NVIC_SystemReset()` + log to NVM (global [6]).

```cpp
template<class T> struct Result {
    ErrorCode error;          // ErrorCode::Ok on success
    T         value;
    [[nodiscard]] bool ok() const noexcept { return error == ErrorCode::Ok; }
};
```

---

## [6] CONCURRENCY & FreeRTOS

- Tasks are created **only** in the composition root (`main.cpp` or an explicit `TaskRunner`).
- Task names: `irrigation_task`, `telemetry_task`, `wifi_task`. Stack is specified explicitly, in **words**, with ≥20 % headroom (global [4]).
- Between tasks — `QueueHandle_t` / `StreamBufferHandle_t`. No global `volatile` mutable structures without explicit synchronisation.
- From an ISR — only `*FromISR()` variants (global [3], [4]).
- The watchdog (`esp_task_wdt`) is reset in a task's main loop and nowhere else.

---

## [7] TESTS

| What | Where | How |
|---|---|---|
| `domain/`, `application/` | `test/test_domain`, `test/test_application` | PlatformIO `native` env, Unity / doctest, fake adapters (`FakeSoilSensor`, `FakeClock`) |
| Drivers (`ChirpSoilSensor` etc.) | `test/test_drivers` | env with a real MCU, integration tests on the bench |
| End-to-end auto-watering | manual | float switch + dry run, verify watchdog (20 s max runtime) |

**Rule:** a new use case ships together with a test in `test_application/`. No test — no merge.

---

## [8] PLATFORMIO CONVENTIONS

`platformio.ini` must contain **three envs**:

```ini
# firmware/coordinator/platformio.ini

[platformio]
default_envs = coordinator

[env:coordinator]               ; main hardware build
platform   = pioarduino espressif32 (stable)
board      = esp32-c6-devkitm-1
framework  = arduino
build_flags = -std=gnu++17 -Wall -Werror -fno-exceptions
lib_extra_dirs = ../shared/domain ../shared/application ../shared/protocol

[env:coordinator-native]        ; host tests for shared + coordinator-app
platform = native
test_framework = unity

[env:coordinator-hwtest]        ; integration tests on the MCU
extends = env:coordinator
build_flags = ${env:coordinator.build_flags} -DGH_HW_TESTS=1
```

```
Commands (with `-d` pointing at the subproject):

pio run   -e coordinator           -d firmware/coordinator
pio test  -e coordinator-native    -d firmware/coordinator
pio test  -e coordinator-hwtest    -d firmware/coordinator
pio run   -e sensor-node           -d firmware/sensor-node
```

- Pin library versions in `lib_deps` via `@1.2.3` or git tag / SHA.
- Do not include headers from `infrastructure/` in `domain/` / `application/` — that breaks the `native` env.

---

## [9] PRESENTATION: MQTT + REST

### MQTT (Home Assistant Discovery)

- Topic scheme: `greenhouse/<device_id>/<entity>/state` · `greenhouse/<device_id>/<entity>/cmd`
- Discovery payload is published once on connect to `homeassistant/<component>/<device_id>_<entity>/config` with `retain=true`.
- Payload structures — dedicated `struct` types + serialisation via `ArduinoJson` with a fixed buffer (`StaticJsonDocument<N>`), no heap.

### REST (ESPAsyncWebServer)

- Routes are registered in `RestApi::registerRoutes(AsyncWebServer&)`. Handlers are thin, delegating to `application/`.
- Responses — JSON via `AsyncResponseStream`. Errors → proper HTTP codes (400/404/409/500/503).
- Auth — bearer token from NVS, checked in a middleware function.

---

## [9.5] SECURITY — MVP TRADE-OFFS

This project is a home DIY build, not production IoT. Below are deliberate trade-offs, documented so a future reader does not mistake the absence of protection for a bug.

### What is NOT protected in Phase B (current)

- **NVS partition is unencrypted.** The Wi-Fi password (`NvsWifiCredsStore`), MQTT credentials (`NvsMqttCredsStore`), Zigbee pairing state, soil calibration — everything is stored in plain text in the `nvs` partition. Anyone with physical access to the board can read flash over USB:
  ```
  esptool.py --port /dev/ttyACM0 read_flash 0x9000 0x6000 nvs_dump.bin
  ```
  and recover the passwords via `nvs_partition_gen.py` / hex inspection.
- **`secrets.hpp` (Zigbee TC link key + ExtPanId)** is compiled into the firmware blob and equally retrievable via `read_flash 0x10000 ...`.
- **Flash encryption and Secure Boot v2 are NOT enabled.** The e-fuse is not burned; the board accepts any firmware over USB.
- **Unsigned OTA updates** (when they arrive in Phase C) — will accept any firmware blob.

### When this is acceptable

- The board is physically located in a protected place (a locked greenhouse / a private home).
- Threat model: "a random guest with a laptop", not "a targeted attacker".
- The Wi-Fi network is a separate IoT VLAN with restricted outbound rules (good practice anyway).

### When to move to production-grade

If the board leaves the trusted environment (shared access, lease, sale) — burn the e-fuse and enable:
- ESP32 Secure Boot v2 (RSA-3072 signature check on the bootloader)
- Flash encryption (AES-XTS over flash content)
- NVS encryption (`CONFIG_NVS_ENCRYPTION=y` + a dedicated `nvs_keys` partition)

This is **irreversible** (e-fuse burn), dramatically complicates re-flashing and debug. Do not enable "just in case" — only when the threat model actually changes.

### Minimum hygiene rules in code

- `secrets.hpp` is in `.gitignore`; only `secrets.hpp.example` is committed (already done).
- Do not log passwords via `Serial.print` (even in debug builds).
- Do not transmit Wi-Fi / MQTT password over MQTT / REST in plaintext.
- The AP-mode provisioning SSID must not expose the full `device_id` (use the last 4 hex chars).
- **The provisioning Soft-AP uses WPA2** with a per-device passphrase derived from
  the MAC (`gh-XXXXXXXX`, last 4 octets), logged via `ILogger::info` at boot. The
  operator notes it once from the serial monitor (or prints it on a sticker)
  before joining the `Greenhouse-Setup-XXXX` AP. This defeats passive sniffing
  of the form POST that carries Wi-Fi + MQTT passwords. The passphrase is not a
  secret per se — anyone with physical access can re-read it via USB-CDC — but
  raising the bar past "any laptop within Wi-Fi range" was cheap.

---

## [10] PRE-COMMIT CHECKLIST

- [ ] `pio run -e coordinator -d firmware/coordinator` — no warnings (`-Werror`)
- [ ] `pio test -e coordinator-native -d firmware/coordinator` — all tests green
- [ ] Stack usage from `.su` files does not exceed 80 % of the allocated stack
- [ ] Map file reviewed for large flash / RAM changes (especially when drivers changed)
- [ ] New use case → has a test in `test_application/`
- [ ] New driver → implements a port from `domain/ports/`, does not surface in `application/` directly
- [ ] No `Arduino.h` / `Wire.h` / `WiFi.h` / `millis()` in `domain/` or `application/`
- [ ] No `new` / `malloc` in runtime code (only in `setup()` via `static`)
- [ ] All `[[nodiscard]]`, `noexcept`, `constexpr`, `override`, `final` placed where they belong
- [ ] No commented-out code; no `TODO` without an issue number

---

## [11] QUICK CHEAT-SHEET — WHERE EVERYTHING LIVES

| You want to… | Add it to |
|---|---|
| New sensor | port in `domain/ports/`, adapter in `infrastructure/drivers/`, wired in `main.cpp` |
| New sensor channel (any node) | `shared/protocol/src/ChannelAttrTable.hpp` + sensor-node `ChannelMappings.hpp` + a bit in `gh::domain::SensorKind` |
| New business rule (e.g. "don't water at night") | `domain/policies/` + test in `test_domain/` |
| New use case (e.g. "auto-calibration of soil_dry/wet") | `application/`, injecting ports, + test in `test_application/` |
| New REST endpoint | `lib/presentation/src/` — pick the matching `Rest*Routes.cpp` per concern (or add a new module and wire it in via the `RestApi` facade) |
| New MQTT entity | `TelemetryPublisher` (state topics) + `HomeAssistantDiscoveryService` (discovery configs) |
| Change a pin / address / timeout | `AppConfig` (NVS) or a `kXxx` constexpr in a single place |

<!-- SPECKIT START -->
For additional context about technologies to be used, project structure,
shell commands, and other important information, read the current plan
<!-- SPECKIT END -->
