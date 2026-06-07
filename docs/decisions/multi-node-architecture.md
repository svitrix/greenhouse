# ADR: Multi-Node Zigbee Architecture

> **Status:** Accepted — implemented as of v2.0.0-multinode
> **Key files:**
> - `firmware/coordinator/lib/application/src/ZigbeeReportRouter.cpp`
> - `firmware/coordinator/lib/infrastructure/src/registry/InMemoryNodeRegistry.hpp`
> - `firmware/coordinator/lib/application/src/IrrigationService.cpp`
> - `firmware/sensor-node/src/main.cpp`

---

## D1 — Coordinator narrows to hub; no local I²C sensors

In the original single-node design, the coordinator read AM2315C and Chirp locally
over I²C. In v2 the coordinator has no local sensors at all.

**Why:**
- Clean separation of concerns: coordinator = network hub + pump + REST + MQTT.
  Sensor reading = sensor-node's job.
- Scaling to N nodes requires the coordinator to treat all sensor data uniformly via
  Zigbee callbacks regardless of source. Special-casing local I²C creates two code
  paths for the same data.
- The coordinator DevKitM-1 is AC-powered, always-on. Wiring soil sensors to it
  implies running cables to the board — impractical beyond 1 plant.

**Consequence:** `IrrigationService` no longer injects `ISoilSensor` / `IAirSensor`.
It reads from `INodeRegistry` which is fed by `ZigbeeReportRouter`.

---

## D2 — IEEE address as primary node key

Nodes are keyed by 64-bit IEEE address (EUI-64), not by MAC suffix, alias, or
sequential ID.

**Why:**
- IEEE address is globally unique and stable across re-pairing (unlike short NWK addr
  which changes on every join).
- Alias is user-editable; keying by alias breaks lookups on rename.
- Sequential IDs require central assignment and create gaps on node removal.

**Trade-off:** IEEE addresses are 8 bytes, hex-rendered as 16 chars in JSON
(`"00:11:22:33:44:55:66:77"`). Accepted — these appear in URLs and MQTT topics only,
not in high-frequency payloads.

---

## D3 — 8-node cap with eviction-of-oldest-offline

`InMemoryNodeRegistry` holds max 8 live node snapshots. When the cap is reached and a
new node joins, the oldest offline node is evicted.

**Why 8:**
- Sufficient for a residential greenhouse (1–6 plants realistic; 8 = comfortable
  headroom).
- 512 KB SRAM: 8 nodes × (registry snapshot + 24-h ring buffers per channel) fits
  comfortably without pressure on the heap.
- Larger fleets (farms, commercial) belong to a different product class.

**Why evict oldest-offline, not reject:**
Silent rejection would cause confusion when swapping a dead node for a new one without
explicitly deleting the old one. Eviction of an offline node is non-destructive from
the user's perspective (the node is already not reporting).

Explicit deletion: `DELETE /api/v2/nodes/{ieee}`.

---

## D4 — Sleepy end-device (Zigbee) vs Wi-Fi sensor nodes

**Alternatives:**
- **A. Wi-Fi MQTT sensor nodes.** No coordinator needed; each node publishes to MQTT
  broker directly.
- **B. Zigbee sleepy end-device** (rx_on_when_idle=false).

**Choice: B.**

| Factor | Wi-Fi MQTT | Zigbee sleepy |
|---|---|---|
| Battery life (1000 mAh) | ~2–5 days (Wi-Fi beacon wakeup) | ~6–18 months (deep sleep, 5 min cycle) |
| RF range through walls | moderate (2.4 GHz) | better (mesh, 2.4 GHz but lower power) |
| Config per node | full Wi-Fi + MQTT credentials | zero config (join coordinator) |
| Number of nodes | limited by router IP table | 8 cap (our choice), protocol supports 65 000+ |
| Implementation complexity | simpler (no Zigbee stack) | requires coordinator + esp-zigbee-sdk |

Battery life is the deciding factor for soil sensors: a node buried next to a plant
pot that needs recharging every week is worse UX than adding a Zigbee coordinator.

---

## D5 — Sensor-node as single-pass linear program (no loop)

The sensor-node never executes `loop()`. Each wake cycle is:
`setup() → probe → read → report → sleep → (RTC wakes, repeats from setup())`.

**Why no loop:**
- Deep sleep on ESP32-C6 is exit + reboot from `setup()`. Persistent state lives in
  RTC memory or NVS, not in stack variables.
- Linear code is simpler to reason about and debug: the program terminates cleanly
  after one cycle.
- FreeRTOS tasks would add scheduling overhead on a device whose entire active window
  is ~2 seconds every 5 minutes.

---

## D6 — ZigbeeReportRouter as the fan-out point

All incoming ZCL attribute reports funnel through one class (`ZigbeeReportRouter`)
which distributes to:
1. `INodeRegistry` (updates live snapshot)
2. `IHistoryStore` (appends to 24-h ring buffer)
3. `ITelemetryQueue` (feeds analytics uploader)

**Why one fan-out class, not three separate callbacks:**
- Single parse of the ZCL payload.
- Atomic update: all three stores reflect the same report simultaneously.
- Adding a new consumer (e.g., an alarm service) = adding one line in `ZigbeeReportRouter`,
  not wiring a new callback into the ZCL stack.

---

## D7 — Atomic v2 cutover, no dual-API period

v1 REST routes (`/api/status`, `/api/sensors`, `/api/history`, `/api/pump`,
`/api/config`, `/api/system`) were deleted in one commit block when v2 landed.
v1 MQTT topics are purged once at first boot via `V1MqttPurge` (publishes empty
retained payloads, NVS-gated so it runs exactly once).

**Why not a compatibility shim:**
- Dual maintenance doubles the surface to keep consistent.
- The only clients of the v1 API are the coordinator's own SPA and the HA MQTT
  integration — both were updated atomically with the REST change.
- Constitution IV: "REST API versions are cut atomically — no dual-API shims."

---

## D8 — SSE over polling for real-time dashboard updates

The operational SPA receives updates via `GET /api/events` (Server-Sent Events)
pushing a dashboard payload every 2 seconds. Polling `GET /api/dashboard` is the
fallback for SSE-unsupported browsers.

**Why SSE over WebSocket:**
- One-directional (server → browser). Pump commands still use REST POST — bidirectional
  is not needed.
- SSE uses standard HTTP, survives proxies and load balancers without special headers.
- Simpler implementation: `AsyncEventSource` in ESPAsyncWebServer is ~20 lines.
- WebSocket would add a stateful handshake, ping-pong keepalive, and binary framing
  for no benefit in this use case.
