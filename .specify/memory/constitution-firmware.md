# Greenhouse ESP32 — Firmware Constitution

> Governs: `firmware/coordinator/` and `firmware/sensor-node/`
> C++17 · ESP-IDF / Arduino framework · FreeRTOS · Zigbee
>
> Parent: [`.specify/memory/constitution.md`](constitution.md)

## Core Principles

### I. Clean Architecture — Inward-Only Dependencies

Dependencies point **inward only**:

```
presentation/  →  application/  →  domain/  ←  infrastructure/
                                                     ↑
                                              main.cpp (composition root)
```

Every function, class, or `#include` that references hardware (`Arduino.h`, `Wire.h`,
`WiFi.h`, `millis()`, `digitalWrite()`, ESP-IDF APIs) MUST live in `infrastructure/`
or `presentation/` — never in `domain/` or `application/`. Port interfaces
(`ISoilSensor`, `IPump`, `IZigbeeNetwork`, …) defined in `domain/ports/` are the only
bridge.

Violations are caught at compile time: the `coordinator-native` env has no
Arduino/IDF headers — a build failure there IS a constitution violation.

**MUST NOT**: import `Arduino.h`, `Wire.h`, `WiFi.h`, or any ESP-IDF header inside
`domain/` or `application/`.

### II. Memory-Safe Embedded Runtime

After `setup()` completes: **no `malloc`, no `new`, no `delete`** — heap fragmentation
causes crashes after hours of continuous operation. No `std::string`, `std::vector`,
`std::map` in the operational loop. All buffers use fixed-size declarations
(`uint8_t buf[256]`) or `static` ring-buffer pools pre-allocated in `setup()`.

**MUST**: use fixed-width types everywhere — `uint8_t`, `uint16_t`, `uint32_t`,
`int32_t`. No plain `int`, `long`, `short`. Only `static_cast` / `reinterpret_cast` —
never C-style casts. Every variable shared between an ISR and main context MUST be
`volatile`. Multibyte volatile access MUST be guarded by disabling IRQ or using
atomic ops. Never `abort()` / `exit()` — use `NVIC_SystemReset()` + error log to NVM.

**MUST NOT**: call `std::string` constructors or any heap container in the loop.

### III. Test-Before-Ship (NON-NEGOTIABLE)

Every new use case in `application/` ships with at least one native test in
`test/test_application/`. Every new domain port ships with a fake adapter in
`test/fakes/`. Every new driver in `infrastructure/drivers/` MUST implement a port
from `domain/ports/` and have a hardware integration test in `test/test_drivers/`.

No branch merges to `main` without `pio test -e coordinator-native` fully green.
Stack usage from `.su` files MUST stay below 80 % of the allocated task stack.

**MUST NOT**: merge a new use case without a test in `test_application/`.

### IV. Safety-First Actuator Control

The pump relay MUST never run longer than `kPumpMaxRuntimeMs` (currently 20 000 ms).
The float switch MUST prevent any pump activation when triggered, regardless of any
irrigation schedule or manual override. These limits live as compile-time constants in
`IrrigationService` / `IrrigationServiceV2` — not configurable via REST or MQTT.

The watchdog MUST be reset only inside the main operational loop — never in ISRs,
one-shot paths, or timer callbacks. All actuator state transitions MUST be logged via
`ILogger::info` or `ILogger::warn`.

**MUST NOT**: bypass the pump runtime guard or float-switch check, even in test builds.

### V. Zigbee Protocol Stability

The ZCL custom soil cluster is `0x0408`. Attribute IDs (soil moisture, soil temp, air
temp, air humidity, battery) are frozen in `shared/protocol/src/ChannelAttrTable.hpp`.
Endpoint numbers MUST NOT change after a node has joined. Report period attribute
`0xFF00` on Basic cluster is the only configurable wire parameter.

`ChannelAttrTable.hpp` is the single source of truth — update it, not scattered magic
numbers. Any change to this file is a MAJOR amendment to this constitution.

---

## Hardware Constraints

### Coordinator — ESP32-C6-DevKitM-1
- SoC: RISC-V 32-bit, 160 MHz, 512 KB SRAM, 4 MB flash
- Power: always-on, USB-5V / wall adapter
- GPIO18 = pump relay · GPIO14 = float switch · GPIO8 = WS2812 status LED

### Sensor-Node — ESP32-C6 SuperMini
- Battery-powered (~1 000–2 000 mAh LiPo), sleepy Zigbee end-device
- AM2315C (air temp + humidity, I²C 0x38) · Chirp! capacitive soil (I²C 0x20)
- Battery ADC on GPIO2 (voltage divider)
- Sleep cycle: setup → read all sensors → Zigbee ReportAttrs burst → read period
  attr → gate sensors off → `esp_deep_sleep_start()`
- Report period: 300 s default · 60–3 600 s configurable

### Compiler & Flags
```
riscv32-esp-elf-g++ -std=gnu++17
-Wall -Wextra -Werror -Wconversion -Wshadow
-fno-exceptions -fno-rtti
-fstack-usage
```

### Coordinator Embedded SPA (Preact)
- Preact + signals — no build step, served from LittleFS
- No CDN dependencies — all assets embedded
- ≤ 200 KB gzipped total; system fonts only
- Design tokens inherit from `services/dashboard` (teal/lime/mint palette)

### Multi-Node Cap
- NodeRegistry: max 8 simultaneous nodes (IEEE-addr keyed)
- Eviction: oldest-offline node removed when cap reached
- Explicit removal: `DELETE /api/v2/nodes/{ieee}`

---

## Governance

Defers to the Master Constitution for amendment procedure and version policy.
Sub-constitution version is independent; bump it when firmware-specific rules change.
Reference: [`.specify/memory/constitution.md`](constitution.md)

**Version**: 1.0.0 | **Ratified**: 2026-06-05 | **Last Amended**: 2026-06-07
