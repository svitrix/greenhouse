# green-house

Zigbee-based greenhouse automation system: two ESP32-C6 firmwares and a React admin SPA.

## What's in the repo

```
firmware/
  shared/           — platform-neutral C++17 libs (gh-domain, gh-app-shared, gh-protocol)
  coordinator/      — ESP32-C6-DevKitM-1, mains-powered; Zigbee coordinator + irrigation + MQTT + REST + embedded SPA
  sensor-node/      — ESP32-C6 SuperMini, battery-powered; AM2315C + Chirp! over I²C, Zigbee sleepy end device
services/
  dashboard/        — React/TS admin UI (served from local dev server, talks to coordinator REST API)
docs/
  hardware/         — canonical hardware reference (pinouts, connection matrices, assembly guide)
archive/legacy-monolith/  — previous single-board layout, reference only
```

## Current state

| Component | Status |
|---|---|
| **Coordinator** | Phase D complete — multi-node pipeline: `InMemoryNodeRegistry` + `InMemoryHistoryStore` + `ZigbeeReportRouter` + `IrrigationService` + full REST API (9 route modules) + HA Discovery (per-node) + analytics uploader + Hue-style hub pairing. Tag: `v2.0.0-multinode`. |
| **Sensor-node** | Phase B complete — AM2315C (air temp/humidity) + Chirp! (soil moisture/temp) over I²C; Zigbee Sleepy End Device with configurable report period; deep sleep between reports. |
| **Dashboard** | React/TS SPA; Vite dev server; talks to coordinator `/api/*`. |

Hardware smoke verification (multi-node pairing, per-node telemetry, auto-irrigation) is pending.

## Quick start

**Coordinator:**

```bash
pio run   -e coordinator           -d firmware/coordinator          # build
pio run   -e coordinator -t upload -d firmware/coordinator          # flash
pio test  -e coordinator-native    -d firmware/coordinator          # host unit tests
pio test  -e coordinator-hwtest    -d firmware/coordinator          # driver tests on MCU
pio device monitor -e coordinator  -d firmware/coordinator          # serial @115200
```

**Sensor-node:**

```bash
pio run   -e sensor-node           -d firmware/sensor-node          # build
pio run   -e sensor-node -t upload -d firmware/sensor-node          # flash
pio test  -e sensor-node-native    -d firmware/sensor-node          # host unit tests
```

**Dashboard (dev):**

```bash
cd services/dashboard
npm install
npm run dev
```

## First-time coordinator provisioning

1. Flash firmware.
2. Board comes up as AP `Greenhouse-Setup-XXXX` (WPA2 passphrase printed on serial at boot).
3. Join AP → captive portal at `http://192.168.4.1/` — fill Wi-Fi, MQTT, soil calibration, admin credentials.
4. Board reboots into operational mode.

To re-enter provisioning: hold the onboard BOOT button for ≥ 3 s right after a reset.

## Key docs

| Topic | File |
|---|---|
| Architecture, code conventions, memory / ISR rules | `CLAUDE.md` (root) |
| Hardware: canonical values (pins, addresses, calibration) | `docs/hardware/reference/canonical-values.md` |
| Hardware: assembly guide | `docs/hardware/coordinator-assembly.md` |
| Hardware: connection matrices | `docs/hardware/connections/` |
| Firmware: coordinator deep dive | `firmware/coordinator/CLAUDE.md` |
| Firmware: sensor-node deep dive | `firmware/sensor-node/CLAUDE.md` |
| BOM and wiring rationale | `COMPONENTS.md` |
