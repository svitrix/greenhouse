# CLAUDE.md — `firmware/`

> Project root file — `/Users/bladerunner/Work/personal_projects/green-house-esp32/CLAUDE.md` (clean architecture, code conventions, error handling, security trade-offs). **Hardware details live at the firmware level** — see the subproject's `CLAUDE.md` (coordinator → DevKitM-1, sensor-node → SuperMini). Full architectural design — `docs/superpowers/specs/2026-05-17-split-coordinator-sensor-node-design.md`.

This directory hosts **two independent firmware projects**, built separately via `pio run -d <subproject>`:

| Subproject                  | Target hardware              | Role                                                                                  | Local CLAUDE.md                          |
|----------------------------|------------------------------|---------------------------------------------------------------------------------------|------------------------------------------|
| [`coordinator/`](coordinator/) | ESP32-C6-DevKitM-1 (5 V wired) | Zigbee Coordinator + WateringPolicy + Wi-Fi → MQTT → Home Assistant + pump relay + float switch | [`coordinator/CLAUDE.md`](coordinator/CLAUDE.md) |
| [`sensor-node/`](sensor-node/) | ESP32-C6 SuperMini (LiPo battery) | Zigbee Sleepy End Device: AM2315C + Chirp on I²C, deep sleep, report period configured from the coordinator | [`sensor-node/CLAUDE.md`](sensor-node/CLAUDE.md) |

Shared code lives in [`shared/`](shared/) (libs `gh-domain`, `gh-app-shared`, `gh-protocol`) and is pulled in by both firmwares via `lib_extra_dirs` in their `platformio.ini`.

---

## Shared technical stack

These facts hold for **both** firmwares. Board-specific things (pinout, on-board peripherals, connectivity profile, partition layout) live in the per-subproject `CLAUDE.md`.

- **MCU:** ESP32-C6 (RISC-V, 1 HP core @ 160 MHz + 1 LP core) · 512 KB SRAM · 4 MB Flash embedded in the chip package · Wi-Fi 6 + BLE 5 + IEEE 802.15.4 (Zigbee 3.0 / Thread 1.3) · Native USB 2.0 **Full-Speed only (12 Mbps)**.
- **Module / chip variant:** ESP32-C6-MINI-1 module with ESP32-C6FH4 chip — same on both DevKitM-1 (coordinator) and SuperMini (sensor-node).
- **Framework:** PlatformIO + `framework = arduino` (Arduino-ESP32 6.x). Where ESP-IDF API is unavoidable (esp-zigbee-lib, deep sleep, gpio_hold), pull it in via `framework = arduino, espidf` provided by pioarduino.
- **OS:** FreeRTOS via Arduino — no direct `vTaskCreate` outside the composition root (`main.cpp`).
- **Storage primitives:** NVS (`Preferences`) for small key/value config. LittleFS only where larger blobs are needed (coordinator).
- **Build flags (baseline):** `-std=gnu++17 -Wall -Werror -fno-exceptions`. Each firmware extends with its own `-W…` set in `platformio.ini` (sensor-node keeps `-Wextra -Wconversion -Wshadow -fno-rtti`; coordinator unflags some because `ESPAsyncWebServer` / `PubSubClient` headers don't compile cleanly under them — documented in `coordinator/platformio.ini`).
- **Hardware reference docs:** [`../COMPONENTS.md`](../COMPONENTS.md) (BOM, wiring rationale, per-board notes), [`../diagram.json`](../diagram.json) (Wokwi schematic), [`../archive/esp-dev-kits-en-master-esp32c6.pdf`](../archive/esp-dev-kits-en-master-esp32c6.pdf) (Espressif esp-dev-kits user guide, board-level reference for DevKitM-1).
- **Reference firmware (for ideas, not a dependency):** [svitrix/svitrix-firmware](https://github.com/svitrix/svitrix-firmware).

---

## ESP32-C6 chip reference (applies to both boards)

This is chip-level — same constraints on DevKitM-1 and SuperMini, because both use the ESP32-C6-MINI-1 module. Board-level details (which of these GPIOs are physically exposed on header pins, what extras the board adds — LEDs, USB-UART bridges, chargers) — see the subproject `CLAUDE.md`.

### Subsystems and the GPIOs they can use

| Subsystem | Signals / GPIO |
|---|---|
| **ADC1** | **only ADC1** on ESP32-C6, **no ADC2** · 7 channels: `CH0..CH6` ↔ `GPIO0..GPIO6` · multi-point calibration. `analogRead()` works **only** on these pins. |
| **LP coprocessor (LP_GPIO)** | sees **only `GPIO0..GPIO7`** as `LP_GPIO0..LP_GPIO7` — anything wired here can be polled from deep-sleep without waking HP-core |
| **LP UART** | RXD=GPIO4, TXD=GPIO5, DTR=GPIO0, DSR=GPIO1, RTS=GPIO2, CTS=GPIO3 |
| **LP I²C** | SDA=GPIO6, SCL=GPIO7 |
| **HP UART0** (`Serial`) | U0TXD=GPIO16, U0RXD=GPIO17 |
| **FSPI (general SPI)** | HD=GPIO4, WP=GPIO5, CLK=GPIO6, D=GPIO7, Q=GPIO2 · CS0..CS5=GPIO16..GPIO21 |
| **SDIO** | CMD=GPIO18, CLK=GPIO19, DATA0..3=GPIO20..GPIO23 |
| **Native USB** | D−=GPIO12, D+=GPIO13 — Full-Speed 12 Mbps · supports USB-CDC and USB-JTAG |
| **JTAG (pads)** | MTMS=GPIO4, MTDI=GPIO5, MTCK=GPIO6, MTDO=GPIO7 (or via native USB-JTAG) |
| **802.15.4 / Wi-Fi 6 / BLE 5** | internal — no GPIO assignment, antenna fixed by the board |

### Strapping pins — critical for boot

`GPIO4` (MTMS) · `GPIO5` (MTDI) · **`GPIO8`** · **`GPIO9`** · `GPIO15`

- Their state at reset determines boot mode and flash behavior. Do NOT attach pull-up / pull-down resistors to them without understanding the effect.
- External loads on these pins must be **high-impedance until reset completes**, otherwise the chip enters Download mode or hangs.
- `GPIO8` is normally already used by an on-board RGB LED (true on both DevKitM-1 and SuperMini).
- `GPIO9` — usually the on-board BOOT button.
- Details — `ESP32-C6 Datasheet > Strapping Pins`.

### Hard constraints that affect firmware

1. **USB native — Full-Speed only (12 Mbps).** Not suitable for High-Speed USB. USB-CDC for logs and USB-JTAG for debug are fine.
2. **ADC only on `GPIO0..GPIO6`.** Any analog sensor wires here. Outside that range, `analogRead()` does not work.
3. **LP features (LP_UART, LP_I2C, LP_GPIO) — only on `GPIO0..GPIO7`.** If you plan deep-sleep with the LP-core polling sensors without waking HP — wire I²C on GPIO6/7, UART on GPIO4/5.
4. **Native USB ↔ GPIO12/13.** If USB-CDC / USB-JTAG is enabled, these pins **cannot** be used as regular GPIO / sensors.
5. **`GPIO10` / `GPIO11` are NOT exposed by the ESP32-C6-MINI-1 module** (internal SPI-flash pins). True on both DevKitM-1 and SuperMini — never reach for these.
6. **Flash is fixed at 4 MB** (embedded in ESP32-C6FH4). Any OTA layout in `partitions.csv` must fit; `platformio.ini → board_build.flash_size = 4MB`.
7. **One HP-core + one LP-core.** Multithreading — only through FreeRTOS tasks on HP-core. `xTaskCreatePinnedToCore(..., core_id=0)` — always core 0.

### "Which pin should I use?" — decision tree

1. Need **ADC** → only `GPIO0..GPIO6`.
2. Need **LP mode** (polling in deep-sleep) → only `GPIO0..GPIO7`, with the correct `LP_*` alias.
3. Is it a **strapping** pin (`GPIO4, 5, 8, 9, 15`)? If yes — avoid it, or guarantee high-Z during reset.
4. **GPIO8** — almost certainly the on-board RGB LED. Use only if you actually need the LED.
5. **GPIO12, GPIO13** — occupy only if USB-CDC / JTAG is not used.
6. **GPIO16, GPIO17** — occupy only if `Serial` logs are routed via USB-CDC or another UART (true on DevKitM-1 where U0 goes through the USB-UART bridge).
7. **GPIO10 / GPIO11** — **unavailable** on both boards.
8. Otherwise — pick a digital-only pin per the subproject's pinout table (`firmware/coordinator/CLAUDE.md` §0 or `firmware/sensor-node/CLAUDE.md` §0).

### Reference docs (chip-level deep dive)

- ESP32-C6 Datasheet — chip characteristics
- ESP32-C6 Technical Reference Manual — memory, registers, peripherals
- ESP32-C6 Hardware Design Guidelines — PCB integration
- ESP32-C6-MINI-1 Datasheet — module

---

## Build / test / flash

```bash
# Coordinator
pio run   -e coordinator           -d firmware/coordinator
pio test  -e coordinator-native    -d firmware/coordinator
pio test  -e coordinator-hwtest    -d firmware/coordinator   # requires a connected MCU
pio run   -e coordinator -t upload -d firmware/coordinator

# Sensor-node
pio run   -e sensor-node           -d firmware/sensor-node
pio test  -e sensor-node-native    -d firmware/sensor-node
pio run   -e sensor-node -t upload -d firmware/sensor-node
```

---

## Current project phase

**Phase A (refactor) — completed** (tag `phase-a-complete`). The codebase was reorganised into a monorepo; the coordinator keeps running as before (reads sensors locally over I²C); the sensor-node is a stub firmware that blinks the onboard WS2812 LED.

**Phase B (Zigbee MVP) — completed.** AM2315C / Chirp drivers live on the sensor-node; `ZigbeeEndDeviceAdapter` ships reports to the coordinator; the coordinator parses standard ZCL frames and routes them into its sensor pipeline.

**Phase C (HA Discovery + manual override) — completed.** Full entity set in Home Assistant, manual pump control via MQTT, captive-portal provisioning, REST + admin auth, watchdog gates on the safety state machine.

**Phase D (multi-node coordinator) — completed 2026-06-05.** The coordinator now runs the multi-node NodeRegistry-driven pipeline: `InMemoryNodeRegistry` + `InMemoryHistoryStore` + `ZigbeeReportRouter` + `IrrigationService` + REST/MQTT/HA Discovery (per-node, IEEE-keyed). The v1 `SensorCache` / `TelemetryPublisher` / `HomeAssistantDiscovery` pipeline has been removed. The SPA was rewritten on `/api`. Hardware smoke verification (Tasks 35 + 43) is pending. Tag: `v2.0.0-multinode`.

See spec §15 "Phased migration plan" and `docs/superpowers/plans/2026-06-01-multinode-coordinator.md` for details.

---

## Dependencies between subprojects

There are **no** direct build-time dependencies — each firmware builds independently. The contact points are:

1. **Shared C++ libs.** Both pull in `shared/domain`, `shared/application`, `shared/protocol` via `lib_extra_dirs`. These libs must not include `Arduino.h`, `Wire.h`, `WiFi.h`, `FreeRTOS.h`. Pure C++17.

2. **Zigbee wire protocol.** The sensor-node emits standard ZCL frames; the coordinator parses them. Cluster / attribute ID alignment is held in `shared/protocol/src/ZclIds.hpp` (placeholder in Phase A; populated in Phase B).

3. **NVS namespaces.** They do not overlap: the coordinator writes to `wifi`, `mqtt`, `soil_calib`, `irrig_cfg`, `zigbee_net`; the sensor-node writes to `zigbee_pair`.

---

## Where to look next

- [`coordinator/CLAUDE.md`](coordinator/CLAUDE.md) — coordinator firmware in detail: current implementation status, MQTT / HA Discovery wiring, Zigbee coordinator role, irrigation safety rules.
- [`sensor-node/CLAUDE.md`](sensor-node/CLAUDE.md) — sensor-node firmware in detail: current state, what Phase B brings, exactly how data flows to the coordinator over Zigbee.
- `../docs/superpowers/specs/2026-05-17-split-coordinator-sensor-node-design.md` — full design with all rationales.
- `../docs/superpowers/plans/2026-05-17-phase-a-monorepo-refactor.md` — completed Phase A plan (history and acceptance).
