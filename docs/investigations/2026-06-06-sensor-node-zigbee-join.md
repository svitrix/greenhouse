# Investigation: sensor-node fails to join Zigbee (handoff)

Date: 2026-06-06. Written for continuation in a fresh chat.

## TL;DR

- **Coordinator is fully working** (WiFi, dashboard, SSE, login, Zigbee network forms,
  permit-join opens). All major fixes flashed and verified on hardware.
- **sensor-node does NOT join.** Root cause **pinpointed** (deep research 2026-06-06,
  see "## UPDATE 2026-06-06" below): the node calls
  `esp_zb_custom_cluster_add_custom_attr()` on the **standard Basic cluster handle**
  returned by `esp_zb_basic_cluster_create()`. The SDK rejects adding a custom
  attribute to a standard-builder cluster → the call returns non-`ESP_OK` →
  `ZB_RETURN_IF_FAIL` returns `NetworkDown` → the node **never reaches network
  steering**. First failing call: [`ZigbeeEndDeviceAdapter.cpp:361`](../../firmware/sensor-node/lib/infrastructure/src/network/ZigbeeEndDeviceAdapter.cpp#L361).
  This is a **local node-firmware bug**, NOT a coordinator / channel / permit-join issue.
  ⚠️ **Correction:** the prior "Phase-2 hypothesis" below blamed the `REPORTING` access
  flag on attr `0xF001`. That is a **red herring** — the first failure is attr `0xFF00`
  (line 361), which has **no** `REPORTING` flag. Fixing only the flag will NOT fix the join.
- It was previously masked: before the partition fix the node crash-looped on a missing
  `zb_storage` partition (abort in `zb_esp_nvram.c:84`), so cluster registration was never
  exercised on hardware.

## Hardware / ports

- Both boards on the laptop over USB.
- **Coordinator** = ESP32-C6-DevKitM-1, UART bridge port `/dev/cu.usbserial-210`
  (Serial app-logs via `log.info` do NOT appear here; only `Serial.printf` + ESP_LOG do).
- **sensor-node** = ESP32-C6 SuperMini, native USB-Serial-JTAG `/dev/cu.usbmodem101`.
  - Sleepy device: boots → tries join → on failure `deep sleep 1 h` → **USB port disappears**.
  - Production firmware has **no USB-CDC**, so `SerialLogger` app-logs go to UART0
    (GPIO16/17, not exposed on SuperMini) → invisible. Only ESP_LOG + panic reach USB.

## The captured node logs (diagnostic USB-CDC build)

With a diagnostic build (`-DARDUINO_USB_CDC_ON_BOOT=1 -DARDUINO_USB_MODE=1`) +
`log_.error("DIAG", ...)` markers, the node consistently logs, ~1.4 s after boot:

```
[  1025][E][esp32-hal-adc.c:208] __analogChannelConfig(): Pin is not configured as analog channel
[  1035][E][Preferences.cpp:47] begin(): nvs_open failed: NOT_FOUND
E (1374) DIAG: platform ok
E (1376) phy_init: load_cal_data_from_nvs_handle: failed to get cal_data(0x1102)
E (1444) zigbee: join failed, deep sleep 1 h
```

Interpretation:
- `DIAG: platform ok` → `esp_zb_platform_config()` **passed**.
- Markers placed later — `basic cluster ok`, `all clusters ok`, `before device_register`,
  `device_register ok`, `steering, timeout=…` — **never printed**.
- `zigbee: join failed` is logged from `main.cpp` (`zb.start()` returned non-`Ok`).
- → `start()` returns `NetworkDown` **between `esp_zb_platform_config` and the bracket
  marker "basic cluster ok"** (i.e., very early in cluster/attr registration), OR somewhere
  between "platform ok" and "before device_register". The bracket markers
  (`basic cluster ok` / `all clusters ok`) were added but **not yet captured** (see blocker).
- `phy_init: failed to get cal_data(0x1102)` = no cached PHY calibration in NVS
  (ESP_ERR_NVS_NOT_FOUND). Likely benign (full calibration runs), noted for completeness.

The early-return path is one of the `ZB_RETURN_IF_FAIL(...)` calls in
`firmware/sensor-node/lib/infrastructure/src/network/ZigbeeEndDeviceAdapter.cpp`
`start()`, in the cluster/endpoint build block (~lines 338–494).

## Phase-2 hypothesis (code review, unconfirmed)

Most-suspect call: the manufacturer-specific custom attributes are added with
`esp_zb_custom_cluster_add_custom_attr()` onto **standard** clusters created by
`esp_zb_basic_cluster_create()` / `esp_zb_zcl_attr_list_create()`, several with
`ESP_ZB_ZCL_ATTR_ACCESS_REPORTING`:
- Basic 0xFF00 (report_period, U32, RW), 0xF001 (present_mask, U32, RO|REPORTING)
- Power 0x0020 battery voltage (U8, RO|REPORTING), 0x0021 battery pct (U8, RO|REPORTING)
- Soil custom cluster

Some esp-zigbee-sdk versions reject `REPORTING` access on custom attrs, or reject
`add_custom_attr` on a standard cluster → returns error → `NetworkDown`. **Unverified** —
needs the bracket markers (or a per-call marker) to confirm which call.

## How to VERIFY a node fix WITHOUT node logs (important)

The node's USB logs are painful to capture, but a successful join is observable from the
**coordinator** (whose USB + REST are reliable):

1. Open permit-join (no auth enforced on device currently):
   `curl -s -X POST -H 'content-type: application/json' -d '{"duration_s":180}' http://192.168.50.200/api/zigbee/permit-join`  → `{"ok":true,...}`
2. Reset the node so it re-steers within the window.
3. Poll `curl -s http://192.168.50.200/api/nodes` — on success `nodes:[]` becomes a node
   with an `ieee`. (`/api/nodes`, `/api/pump`, `/api/auto-water/state`, `/api/dashboard`
   all return 200 **without** auth right now.)

So: pinpoint+fix the failing cluster call, flash node, then verify via `/api/nodes`.

## The blocker: flashing / observing the SuperMini

- Node sleeps → USB port `usbmodem*` vanishes (1 h after a failed join).
- To flash: node must be in **download mode**. Reliable recipe that worked here:
  1. Wait until the node is asleep (`/dev/cu.usbmodem*` absent).
  2. **Hold BOOT**, tap **RESET**, **keep holding BOOT** through the whole
     `Connecting....` + write (~20 s). Releasing BOOT early → "No serial data received".
- Catch-and-flash loop used (pre-build first so flash starts instantly):
  ```bash
  cd firmware/sensor-node
  PLATFORMIO_BUILD_FLAGS="-DARDUINO_USB_CDC_ON_BOOT=1 -DARDUINO_USB_MODE=1" pio run -e sensor-node   # pre-build
  # then, with node held in download mode:
  P=$(ls /dev/cu.usbmodem* | head -1)
  PLATFORMIO_BUILD_FLAGS="-DARDUINO_USB_CDC_ON_BOOT=1 -DARDUINO_USB_MODE=1" pio run -e sensor-node -t upload --upload-port "$P"
  ```
- Reading logs: connect pyserial with reopen-on-drop the instant the port appears, and
  press RESET so the boot logs flush while connected. USB-CDC drops TX written before a host
  is attached, so boot logs are easily lost — connect fast, or rely on the coordinator-side
  verification above.
- `-DGH_NODE_NO_SLEEP` (keeps USB up) was tried but produced **no output** — the tight
  re-logging loop appears to block USB-CDC TX when no host is attached. Prefer the normal
  (sleeping) build + catch boot logs, OR verify via coordinator.

## Suggested next steps (fresh chat)

1. Flash the node diagnostic build that ALSO has bracket markers
   (`basic cluster ok` after the basic cluster, `all clusters ok` before `ep_list`) — already
   in the code. Capture boot logs → see which group fails.
2. If needed, add a marker after EACH `ZB_RETURN_IF_FAIL` in the cluster block to pinpoint
   the exact call.
3. Fix that call (likely the custom-attr / REPORTING-flag registration). Verify via
   coordinator `/api/nodes`.
4. Revert diagnostics (see below).

## Uncommitted changes made this session

### Coordinator — KEEP (real fixes, flashed + verified)
- `firmware/coordinator/partitions.csv` — added `zb_storage`(16K)+`zb_fct`(4K); placed before
  `littlefs` so PlatformIO's image builder still targets `littlefs`.
- `firmware/coordinator/src/main.cpp` — LittleFS mount label `"littlefs"`; static AP password
  `"GreenHouse"`; `device_id` made `static` + `esp_read_mac()`; `esp_coex_wifi_i154_enable()`
  (THE WiFi↔Zigbee coexistence fix — WiFi throughput was collapsing); Wi-Fi-aware status-LED
  arbitration; `sse_task` + `AsyncEventSource /api/events` + `GET /api/dashboard`; supervisory
  WiFi reconnect in `coordinator_task`; `#ifdef GH_DIAG_NO_ZIGBEE` guard (inert in normal build).
- `WifiStaAdapter.{hpp,cpp}` — `setAutoReconnect(true)`, `WiFi.reconnect()` on runtime drop,
  `setSleep(false)`, `connected_once_` atomic, RSSI log on connect.
- `WifiSoftApAdapter.cpp` — `esp_read_mac()`.
- `ZigbeeBindingTable.{hpp,cpp}` — `shortAddrFor()` (O(8)); `Esp32ZigbeeNetwork.cpp` uses it
  (was O(65534) scan in `requestLeave`).
- `lib/presentation/src/{Ws2812StatusLed,StatusLedService,SystemStatus,DashboardViewBuilder}` +
  `shared/domain/src/ports/IRgbLed.hpp` (new) + tests.
- `lib/application/src/CoordinatorConfig.hpp` — LED/SSE constants.
- `lib/infrastructure/src/network/ProvisioningWebServer.cpp` — MQTT made optional.
- `shared/domain/src/ports/INodeHistoryStore.hpp` — `kHistoryMaxPointsPerSeries` 256→128 (RAM).
- `shared/protocol/src/ZigbeeNetwork.hpp` — `kInitialPermitJoinMs` 60s→254s.
- web: `App.tsx` (auth probe+login+SSE), `api/auth.ts` (new), `api/client.ts` (auth header +
  401→login + `getDashboard`), `routes/Login/*` (new), `state/useDashboard.ts` (SSE),
  `routes/Settings/Settings.tsx` (auth-aware), `routes/Dashboard/Dashboard.tsx` (sign-out),
  `api/schemas.ts`/`types.ts`, tests, `web/CLAUDE.md`.

### sensor-node — mixed
- `firmware/sensor-node/partitions.csv` — added `zb_storage`+`zb_fct`. **KEEP** (real fix).
- `lib/infrastructure/src/network/ZigbeeEndDeviceAdapter.cpp` — `log_.error("DIAG", ...)`
  markers (platform ok / basic cluster ok / all clusters ok / before device_register /
  device_register ok / steering, timeout / no join after Nms). **REVERT after the bug is found.**
- `src/main.cpp` — `#ifdef GH_NODE_NO_SLEEP` guards around the deep-sleep calls. Inert in a
  normal build; **revert or keep behind the flag.**
- USB-CDC was enabled only via `PLATFORMIO_BUILD_FLAGS` (NOT in `platformio.ini`) → production
  builds are unaffected.

### Currently flashed
- Coordinator: latest (all fixes + SSE). Good.
- sensor-node: a **diagnostic** build (USB-CDC + DIAG markers, sleeping). Re-flash a clean
  production build once the bug is fixed.

## Key facts to not re-derive
- WiFi↔Zigbee on one C6 shares the 2.4 GHz radio; coexistence MUST be enabled with
  `esp_coex_wifi_i154_enable()` (done on coordinator) or WiFi throughput collapses (~4 KB/s,
  large transfers truncate). This was the cause of the "blank dashboard / ERR_CONTENT_LENGTH_MISMATCH".
- Shared Zigbee config (`shared/protocol/src/ZigbeeNetwork.hpp` + `secrets.hpp`): channel 25,
  shared TC link key — coordinator and node match. Not the cause of the join failure.
- `/api/*` on the coordinator currently answer 200 **without** Basic auth (middleware not
  effectively enforcing) — separate issue; the SPA login still works as a UX layer.

---

# UPDATE 2026-06-06 — Root cause pinpointed (deep research)

Method: systematic-debugging + 5 parallel read-only agents (node `start()` path, working
coordinator reference, shared-config matching, esp-zigbee-sdk API contract, LED hardware),
then hand-verification of the linchpin claim against the actual source and the **installed**
SDK header. No hardware was flashed this session — verification plan is in §V below.

## I. The bug, in one sentence

The node builds its **Basic cluster (0x0000)** with the *standard-cluster builder*
`esp_zb_basic_cluster_create()`, then tries to bolt manufacturer attributes onto that handle
with `esp_zb_custom_cluster_add_custom_attr()`. **The SDK rejects adding a custom attribute to
a standard-builder cluster handle** (returns non-`ESP_OK`); `ZB_RETURN_IF_FAIL` converts that to
`ErrorCode::NetworkDown` and bails out **before** `esp_zb_device_register()` and **before**
network steering. The node therefore never even gets on the air.

## II. Evidence chain (why this is the cause, not a guess)

1. **Captured node log** (diagnostic build): `DIAG: platform ok` prints, but the very next
   bracket marker `basic cluster ok` does **not**. So the failure is strictly **inside the
   Basic-cluster block**, [lines 341–384](../../firmware/sensor-node/lib/infrastructure/src/network/ZigbeeEndDeviceAdapter.cpp#L341).
2. **The `ZB_RETURN_IF_FAIL`-wrapped calls in that block**, in execution order:
   - L349 `esp_zb_basic_cluster_add_attr(MANUFACTURER_NAME)` — **correct** standard API → passes
   - L354 `esp_zb_basic_cluster_add_attr(MODEL_IDENTIFIER)` — **correct** standard API → passes
   - **L361 `esp_zb_custom_cluster_add_custom_attr(basic_attrs, 0xFF00, U32, READ_WRITE, …)`** — **first misuse**
   - L374 `esp_zb_custom_cluster_add_custom_attr(basic_attrs, 0xF001, U32, READ_ONLY|REPORTING, …)` — same misuse
   - L381 `esp_zb_cluster_list_add_basic_cluster(...)`
   The first two are the SDK-sanctioned way to add those exact standard attrs, so they pass.
   The **first** call that can fail is **L361** → that is the failing call.
3. **Asymmetry that proves it** — only the Basic cluster uses a standard builder; the others
   that *also* call `add_custom_attr` use the **generic** list `esp_zb_zcl_attr_list_create()`,
   which the SDK *does* accept for custom attrs:
   - Basic: `esp_zb_basic_cluster_create()` ([L347](../../firmware/sensor-node/lib/infrastructure/src/network/ZigbeeEndDeviceAdapter.cpp#L347)) → **standard builder → add_custom_attr rejected** ✗
   - Power Config: `esp_zb_zcl_attr_list_create(0x0001)` ([L391](../../firmware/sensor-node/lib/infrastructure/src/network/ZigbeeEndDeviceAdapter.cpp#L391)) → generic list (would pass the add, see §IV) ✓-ish
   - Soil 0x0408: `esp_zb_zcl_attr_list_create(0x0408)` ([L446](../../firmware/sensor-node/lib/infrastructure/src/network/ZigbeeEndDeviceAdapter.cpp#L446)) → generic/custom list → correct ✓
   The log failing **in the Basic block specifically** matches exactly the one cluster built the
   wrong way. This is not a coincidence.
4. **Working coordinator confirms the pattern**: the coordinator joins/forms fine and registers
   **zero clusters/attributes** on its own endpoint (empty cluster list, pure listener,
   `ZigbeeCoordinatorAdapter.cpp` ~L387–402). It never exercises `add_custom_attr`, so it never
   hits this bug — which is why "coordinator works, node doesn't."
5. **SDK API contract** (verified against the installed header, not just docs): historically and
   in 2.x, `esp_zb_custom_cluster_add_custom_attr` is for **custom** clusters; adding to a
   standard cluster returns "not a custom cluster" / `ESP_ERR_INVALID_ARG`
   (espressif/esp-zigbee-sdk issues #125, #405). The `REPORTING` access flag itself is **legal**
   (issues confirm it's used in official examples) — so it is **not** the cause.

**Confidence:** High. Log evidence localizes to the Basic block; only one call there can fail;
the API contract explains why; the working coordinator is the negative control. The only thing
not yet captured on hardware is a *per-call* marker proving 361 vs 374 — but **both fail for the
same reason and take the same fix**, so pinpointing 361-vs-374 does not change the remedy.

## III. The fix (Basic cluster)

The installed SDK **already exposes the correct APIs** — verified in
`~/.platformio/packages/framework-arduinoespressif32-libs/esp32c6/include/espressif__esp-zigbee-lib/include/esp_zigbee_attribute.h`:
- L758 `esp_zb_cluster_add_attr(attr_list, cluster_id, attr_id, type, access, value)` — std manuf code
- L776 `esp_zb_cluster_add_manufacturer_attr(attr_list, cluster_id, attr_id, manuf_code, type, access, value)`
- L117 `esp_zb_power_config_cluster_add_attr(attr_list, attr_id, value)` — for the battery attrs

So this is esp-zigbee-lib **2.x** (these symbols are 2.x additions). Replace the two
`add_custom_attr` calls on `basic_attrs` (L361, L374) with the cluster-aware API that takes the
`cluster_id` explicitly so the SDK validates against the right cluster:

```cpp
// L361 — report_period 0xFF00 (U32, RW)
ZB_RETURN_IF_FAIL(esp_zb_cluster_add_attr(
    basic_attrs,
    static_cast<uint16_t>(gh::protocol::kClusterBasic),      // 0x0000
    gh::protocol::kAttrBasicReportPeriodS,                   // 0xFF00
    ESP_ZB_ZCL_ATTR_TYPE_U32,
    ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE,
    &s_report_period_s));

// L374 — sensors_present_mask 0xF001 (U32, RO[+REPORTING])
ZB_RETURN_IF_FAIL(esp_zb_cluster_add_attr(
    basic_attrs,
    static_cast<uint16_t>(gh::protocol::kClusterBasic),      // 0x0000
    gh::protocol::kAttrBasicSensorsPresentMask,              // 0xF001
    ESP_ZB_ZCL_ATTR_TYPE_U32,
    ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY,                       // see note on REPORTING below
    &s_sensors_present_mask));
```

Notes:
- **`esp_zb_cluster_add_attr` uses the *standard* manufacturer code** (non-manuf-specific custom
  attr). The coordinator's report parser keys only on `(endpoint, cluster, attr_id)` and does
  **not** check a manufacturer code (`ZigbeeCoordinatorAdapter.cpp` action handler), so the
  reports still decode. Registering these as *true* manuf-specific attrs (`esp_zb_cluster_add_
  manufacturer_attr` with `0xFFEE`) is an optional later enhancement and brings the manuf-code
  read/set pitfalls of issues #278/#405 — **do not** do it just to join.
- **REPORTING flag**: the node sends **manual** `Report Attributes` (`ZigbeeReportMapper` →
  `reportAttribute()`), it does **not** use SDK auto-reporting (which additionally requires a
  binding and post-`device_register` `config_report`, see issue #728). So the
  `ESP_ZB_ZCL_ATTR_ACCESS_REPORTING` flag is **not needed** for the data to flow and can be
  dropped to keep things minimal; keeping it is harmless at create time. The crash was never
  about this flag.

## IV. Secondary latent bug — fix it in the SAME pass (Power Config block)

Once Basic is fixed, the **next** block to exercise is Power Config ([L390–410](../../firmware/sensor-node/lib/infrastructure/src/network/ZigbeeEndDeviceAdapter.cpp#L390)).
It adds **standard** attrs (`0x0020` BatteryVoltage, `0x0021` BatteryPercentageRemaining) through
`add_custom_attr` on a generic list, then `esp_zb_cluster_list_add_power_config_cluster()`. The
generic list may *accept* the adds (so it may not crash), but feeding standard attr IDs through
the custom path and presenting it as the real Power Config cluster is malformed and is the kind
of thing `esp_zb_device_register()` validation can still reject. Convert it to the proper builder
to avoid a second hardware round-trip:

```cpp
esp_zb_power_config_cluster_cfg_t power_cfg{};                  // defaults are fine
esp_zb_attribute_list_t* power_attrs =
    esp_zb_power_config_cluster_create(&power_cfg);
ZB_RETURN_IF_FAIL(esp_zb_power_config_cluster_add_attr(
    power_attrs, gh::protocol::kAttrBatteryVoltage,            &s_battery_voltage_zcl));   // 0x0020
ZB_RETURN_IF_FAIL(esp_zb_power_config_cluster_add_attr(
    power_attrs, gh::protocol::kAttrBatteryPercentageRemaining,&s_battery_pct_zcl));       // 0x0021
ZB_RETURN_IF_FAIL(esp_zb_cluster_list_add_power_config_cluster(
    cluster_list, power_attrs, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
```

The Soil cluster (0x0408, [L445–457](../../firmware/sensor-node/lib/infrastructure/src/network/ZigbeeEndDeviceAdapter.cpp#L445)) is built as a generic/custom list and added via
`add_custom_cluster` — that is the **correct** pattern for a custom cluster; leave it as-is.

## V. Verification plan (no node logs needed)

The fix must be proven, not assumed. Per the handoff, a successful join is observable from the
**coordinator** even though the node has no reliable USB logs:

1. Apply §III + §IV, build: `pio run -e sensor-node -d firmware/sensor-node` (must stay `-Werror` clean).
2. Flash the node (BOOT-hold / RESET recipe from the handoff blocker section).
3. Open permit-join: `curl -s -X POST -H 'content-type: application/json' -d '{"duration_s":180}' http://192.168.50.200/api/zigbee/permit-join` → `{"ok":true}`.
4. Reset the node so it re-steers inside the window.
5. Poll `curl -s http://192.168.50.200/api/nodes` — **success = `nodes:[]` becomes a node with an `ieee`**, and channel samples start arriving (`/api/dashboard`).
6. **Belt-and-suspenders, in-firmware:** before flashing the fix, add a single per-call DIAG
   marker between L357→L361 and L366→L374 (or just trust the convergent diagnosis). If after the
   fix the node *still* fails before `device_register ok`, the per-call markers + the §IV block
   are the next things to look at.
7. Revert the `log_.error("DIAG", …)` markers and the `GH_NODE_NO_SLEEP` guard (handoff
   "Uncommitted changes" list) once the join is confirmed; re-flash a clean production build.

This bug would have been **instantly diagnosable** with the LED indicator described next — which
is the second half of this investigation.

---

# LED status indicator (blink codes) — design

**Motivation (directly from the request):** the SuperMini sensor-node has **no reliable log
channel** in production — no USB-UART bridge, USB-CDC disabled, app logs go to an unexposed UART
(GPIO16/17). When a join fails the board just deep-sleeps for 1 h and the USB port vanishes. The
on-board **WS2812 RGB LED on GPIO8** is the one always-available output. Used as a blink-code
indicator it becomes the **production-observability substitute for the missing serial logs** —
and, notably, it would have told us at a glance that *this* bug was a stack/registration failure
(node never reached the air) rather than a permit-join/RF problem.

## A. Hardware & what already exists

- **Node LED:** on-board WS2812 on **GPIO8** (strapping pin, boot-safe as output —
  `docs/hardware/reference/canonical-values.md`, `firmware/sensor-node/CLAUDE.md §0`).
  **No LED driver exists on the node yet** (the "Phase A blink stub" is gone).
- **Reusable abstractions already in the repo** (coordinator side):
  - Port `gh::domain::IRgbLed` — `firmware/shared/domain/src/ports/IRgbLed.hpp` (`setColor(r,g,b)`).
  - Adapter `Ws2812StatusLed` — coordinator `lib/infrastructure/src/drivers/`; drives the LED via
    Arduino `rgbLedWrite(gpio, r, g, b)` (**handles RMT internally — no manual RMT setup**) with a
    brightness-percent scale.
  - `StatusLedService` + `SystemStatus` enum + a `Style{r,g,b,period_ms,on_ms}` blink vocabulary
    and `arbitrate(...)` — coordinator `lib/application/src/status/`.

## B. Why the coordinator's LED service can't be dropped in as-is

The coordinator's `StatusLedService` is **tick-driven** (`tick(now_ms)` called every ~100 ms from
a FreeRTOS task that runs forever). The sensor-node has **no such loop**: `setup()` runs exactly
one `SensorCycle::runOnce()` and then `DeepSleepClock::sleepFor()` — `loop()` is unreachable. And
**deep sleep powers down the RMT peripheral**, so the LED is dead the instant we sleep.

→ The node needs a **blocking, one-shot blink emitter** that runs a short pattern to completion
**before** each `sleepFor()`, not a background ticker. This is the inverse of the coordinator model
but reuses the same `IRgbLed` port and color language.

## C. Proposed blink-code chart (operator-facing)

Colors follow the coordinator's language (green = ok, red = fault, amber = attention, blue =
commissioning). Each code is emitted **once**, right before the terminal `sleepFor()`/sleep on that
path, then the LED goes dark for the whole sleep.

| Event (node state)                                              | Where (`src/main.cpp`)        | Blink code                              | Operator action |
|----------------------------------------------------------------|-------------------------------|-----------------------------------------|-----------------|
| **Joined + reported OK** (normal cycle)                        | after `cycle.runOnce()`, before sleep (~L95–100) | **1× short green** (~200 ms)            | none — node is healthy |
| **Zigbee stack / cluster-registration failed** (THE current bug — `start()` `NetworkDown` before steering) | join-fail branch (~L79–90)    | **3× fast magenta** (150 ms on/off)     | re-flash fixed firmware (this is a firmware bug, not RF) |
| **Steering timeout / no parent** (radio OK, no coordinator / permit-join closed) | join-fail branch (~L79–90)    | **3× red** (300 ms on/off)              | open permit-join on coordinator, reset node |
| **TC link-key mismatch** (`ZigbeeTrustCenterMismatch`; pairing wiped) | TC-mismatch branch (~L80–82)  | **2× amber** (400 ms on/off)            | check TC key / coordinator re-flash; node will re-steer next boot |
| **Sensor power-rail init failure** (hardware, before networking) | `rail.init()` fail (~L52–56)  | **solid red ~1 s**                      | check GPIO4 gate / sensor wiring |
| *(optional)* **Joined but all sensors failed** (presentMask == 0) | after cycle, before sleep     | **2× yellow**                           | check I²C / sensor power |

Magenta-vs-red is the key diagnostic split: **magenta = "I never reached the air" (firmware)**,
**red = "I reached the air but nobody let me in" (RF/permit-join)**. Today both return the same
`NetworkDown` and are indistinguishable — see §E.

## D. Where to emit (verified insertion points)

`firmware/sensor-node/src/main.cpp` (~L29–103), all **before** `sleeper.sleepFor(...)`:
- `rail.init()` failure path (~L52–56) → rail-fault code.
- `const auto join_result = zb.start(...)` (~L78) and its result handling (~L79–92):
  TC-mismatch (~L80–82) vs generic fail (~L84) → the two/three join codes.
- after `cycle.runOnce()` (~L95), before `sleepFor(sleep_ms)` (~L100) → success pulse.
- The LED is **off during deep sleep** (GPIO8 reverts to high-Z; no `gpio_hold` needed).

## E. Recommended companion change — split the join `ErrorCode`

Right now `ZigbeeEndDeviceAdapter::start()` returns `ErrorCode::NetworkDown` for **both** the
cluster-registration failure (every `ZB_RETURN_IF_FAIL`) **and** a real steering timeout. The LED
(and the logs) therefore can't tell "firmware never started the stack" from "no coordinator
found." Introduce distinct codes so the node can pick the right blink **and** so logs are unambiguous:
- e.g. `ZigbeeStackInitFailed` (returned by the `ZB_RETURN_IF_FAIL` registration path) vs
  `ZigbeeJoinTimeout` (returned when steering exhausts `kZbSteeringTimeoutMs`).
- This is cheap (one enum + a couple of return sites) and turns the LED into a genuine diagnostic.
  Had it existed, this entire investigation would have started from "magenta = registration
  failed" instead of capturing USB-CDC logs by hand.

## F. Implementation constraints (embedded rules)

- **No `delay()` > 10 ms in the hot path** (`firmware/sensor-node/CLAUDE.md §7`). The blink codes
  live on **terminal pre-sleep paths**, not the hot path — a bounded ≤ ~1.5 s blink right before a
  1 h sleep (failure) or a single ~200 ms pulse before normal sleep is the same category as the
  already-allowed rail-warmup wait. Keep total LED-on time small; the failure paths are rare.
- **Battery:** WS2812 ≈ 20 mA/channel at full. Use the existing brightness-percent scale (≤ ~30 %),
  prefer single-channel colors, avoid white. A 200 ms green pulse @30 % once per 60 s wake is
  < 1 mAh/day — negligible on a 1000–2000 mAh LiPo. Failure blinks cost nothing (then 1 h sleep).
- **Reuse, don't duplicate:** lift `Ws2812StatusLed` into a shared infra location (both boards use
  the identical `rgbLedWrite`) or add a thin node adapter implementing `IRgbLed`; add a small
  one-shot `StatusBlinker`/`blinkCode()` helper for the node (blocking N×on/off), wired in the
  composition root (`main.cpp`) per the project's clean-architecture rules. The pattern table
  belongs next to `SensorNodeConfig` constants, not hard-coded inline.

> Scope note: the blink indicator is **independent** of the join fix in §III–IV. The node can be
> fixed and verified first (fastest path to a working join), then the LED added as a follow-up that
> pays for itself the next time anything goes wrong in the field.

---

# FIX PLAN — 2026-06-06 (verified + adversarially reviewed)

Produced by a verify → draft → adversarial-review workflow, then hand-checked against the live
source and the installed esp32c6 `esp-zigbee-lib` headers. All line numbers below were read from
the **current** working tree (the diagnostic build with DIAG markers). **Re-anchor by symbol, not
line, after each edit** — every hunk shifts the lines below it.

## Ordering principle (read first)

Ship the **smallest change that yields a working join first**, verify on hardware, then layer on
the rest. Three independent, separately-revertible commits:

| Commit | Scope | Depends on | Provable by |
|---|---|---|---|
| **1 — Join fix** | Step 1 (Basic cluster) [+ Step 3 Power Config only if Step 2 says so] | nothing | hardware `/api/nodes` |
| **2 — ErrorCode split** | Step 4 | commit 1 merged | native test + build |
| **3 — LED indicator** | Step 5 | commit 2 (uses the new codes) | native test + bench blink |

Do **not** bundle the Power Config rewrite into commit 1 unless the bench proves it's needed
(§Step 2). Power Config currently uses the *valid* generic-list + `add_custom_attr` pattern (same
as the soil cluster) — it is **not** obviously broken; only the Basic cluster is. Bundling a
speculative second ZCL change makes a failed join un-bisectable.

## Step 0 — Instrument the failing call (do this with Step 1, revert after)

The root cause is high-confidence but the **actual `esp_err_t` was never captured** — `ZB_RETURN_IF_FAIL`
discards it. Before declaring "verified," surface it during the hunt. Temporarily change the macro
body (`ZigbeeEndDeviceAdapter.cpp:224-230`) to log the errno name:

```cpp
#define ZB_RETURN_IF_FAIL(call)                                       \
    do {                                                              \
        const esp_err_t _zb_err = (call);                             \
        if (_zb_err != ESP_OK) {                                      \
            log_.error("DIAG", esp_err_to_name(_zb_err));  /* TEMP */ \
            return gh::domain::ErrorCode::NetworkDown;                \
        }                                                             \
    } while (0)
```

Expectation from the diagnosis: `ESP_ERR_INVALID_ARG` (`0x102 / 258`) printed once, with no
`basic cluster ok` after it. This single line confirms the root cause empirically. It is replaced
for good in Step 4 (and the `esp_err_to_name` log is removed by the §Revert-gate grep).

## Step 1 — Basic cluster fix (the actual join bug)

**File:** `firmware/sensor-node/lib/infrastructure/src/network/ZigbeeEndDeviceAdapter.cpp`

**Why:** the Basic list is built by the **standard builder** `esp_zb_basic_cluster_create()` (L347);
`esp_zb_custom_cluster_add_custom_attr()` on that handle (L361 `0xFF00`, L374 `0xF001`) is rejected.
**Use the plain generic adder `esp_zb_cluster_add_attr`** (verified `esp_zigbee_attribute.h:758`):

```c
esp_err_t esp_zb_cluster_add_attr(esp_zb_attribute_list_t *attr_list, uint16_t cluster_id,
                                  uint16_t attr_id, uint8_t attr_type, uint8_t attr_access,
                                  void *value_p);
```

> **Plain `esp_zb_cluster_add_attr`, NOT `esp_zb_cluster_add_manufacturer_attr`** — this was the
> top adversarial-review finding (C1). The report path writes the live value with the **plain**
> setter `esp_zb_zcl_set_attribute_val(ep, cluster, SERVER_ROLE, attr_id, data, /*check=*/false)`
> at `ZigbeeEndDeviceAdapter.cpp:592` — **no manuf_code**. If these attrs were registered in the
> SDK's separate manufacturer-keyed store (via `add_manufacturer_attr`), that plain setter would
> not find them and the report would send stale/zero — exactly the failure the existing comment at
> L368-373 warns about. Registering them plain keeps the store the plain setter reads. The
> coordinator decodes on `(endpoint, cluster, attr_id)` only and ignores the manuf code
> (`ZclSensorMapper`, `ZigbeeCoordinatorAdapter` report handler), so interop is unaffected.

### Edit 1a — `0xFF00` report_period (current L361-366)

```cpp
        // Was: esp_zb_custom_cluster_add_custom_attr(basic_attrs, 0xFF00, U32, READ_WRITE, ...)
        // The standard Basic builder rejects add_custom_attr (SDK returns
        // ESP_ERR_INVALID_ARG). esp_zb_cluster_add_attr stores it in the plain
        // attribute list that reportAttribute()'s set_attribute_val reads.
        ZB_RETURN_IF_FAIL(esp_zb_cluster_add_attr(
            basic_attrs,
            static_cast<uint16_t>(gh::protocol::kClusterBasic),
            gh::protocol::kAttrBasicReportPeriodS,
            ESP_ZB_ZCL_ATTR_TYPE_U32,
            ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE,
            &s_report_period_s));
```
Keep `READ_WRITE` — the coordinator writes `0xFF00`.

### Edit 1b — `0xF001` sensors_present_mask (current L374-379)

```cpp
        ZB_RETURN_IF_FAIL(esp_zb_cluster_add_attr(
            basic_attrs,
            static_cast<uint16_t>(gh::protocol::kClusterBasic),
            gh::protocol::kAttrBasicSensorsPresentMask,
            ESP_ZB_ZCL_ATTR_TYPE_U32,
            ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY,        // REPORTING flag dropped — see note
            &s_sensors_present_mask));
```

> **Drop `ESP_ZB_ZCL_ATTR_ACCESS_REPORTING`** (review M4): the firmware sends **manual**
> `esp_zb_zcl_report_attr_cmd_req()` (L626) — SDK auto-reporting/binding is never configured, and
> `set_attribute_val(..., check=false)` means access bits aren't enforced on write either. The flag
> is dead. Keeping it is harmless; dropping it removes a misleading "auto-reporting is configured"
> signal. (Same applies to the Power/Soil attrs; touch those only in their own steps.)

## Step 2 — Build, flash, verify the join (the gate)

This proves the SDK accepts the new calls **and** decides whether Step 3 is needed.

```bash
pio run -e sensor-node -d firmware/sensor-node            # must stay -Werror + strict clean
pio run -e sensor-node -t upload -d firmware/sensor-node  # BOOT/RESET recipe if unresponsive
```
Then, per §V of the investigation:
```bash
curl -s -X POST -H 'content-type: application/json' -d '{"duration_s":180}' \
  http://192.168.50.200/api/zigbee/permit-join          # -> {"ok":true}
# reset the node so it re-steers inside the window, then:
curl -s http://192.168.50.200/api/nodes                 # success = a node with an "ieee" appears
```

**Decision:**
- **Joins** → the Basic fix was sufficient; Power Config was fine as-is. **Skip Step 3.** Optionally
  do Step 3 later as a non-urgent correctness cleanup (battery attrs through the typed builder).
- **Still fails** → read the DIAG marker sequence + the Step-0 errno. If `basic cluster ok` now
  prints but `all clusters ok` / `device_register ok` do not, the failure moved to Power Config or
  `device_register` validating the whole EP → **do Step 3**. If it dies at EP2
  (`esp_zb_temperature_meas_cluster_create`, the correct typed builder already), investigate that
  separately.

## Step 3 — Power Config cluster fix (CONDITIONAL — only if Step 2 still fails past Basic)

**File:** same adapter, current **L389-410**. Convert the generic-list + `add_custom_attr` for the
**standard** battery attrs to the typed builder (verified: `esp_zb_power_config_cluster_create`
`esp_zigbee_cluster.h:59`; `esp_zb_power_config_cluster_add_attr(list, attr_id, value_p)`
`esp_zigbee_attribute.h:117`; cfg struct `esp_zb_power_config_cluster_cfg_t` is all-POD, `{}` zero-init valid):

```cpp
    {
        // create() also seeds default mains-voltage/-frequency attrs — harmless
        // on a battery node, ignored by the coordinator.
        esp_zb_power_config_cluster_cfg_t power_cfg{};
        esp_zb_attribute_list_t* power_attrs =
            esp_zb_power_config_cluster_create(&power_cfg);
        ZB_RETURN_IF_FAIL(esp_zb_power_config_cluster_add_attr(
            power_attrs, gh::protocol::kAttrBatteryVoltage,            &s_battery_voltage_zcl));
        ZB_RETURN_IF_FAIL(esp_zb_power_config_cluster_add_attr(
            power_attrs, gh::protocol::kAttrBatteryPercentageRemaining, &s_battery_pct_zcl));
        ZB_RETURN_IF_FAIL(esp_zb_cluster_list_add_power_config_cluster(
            cluster_list, power_attrs, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    }
```
`s_battery_voltage_zcl` / `s_battery_pct_zcl` are both `uint8_t` (L69-70) matching ZCL U8 0x0020/0x0021.
**Fallback** if the typed adder rejects either ID on the bench: generic list + the 6-arg
`esp_zb_cluster_add_attr(power_attrs, kClusterPowerConfiguration, attr_id, ESP_ZB_ZCL_ATTR_TYPE_U8,
ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY, &value)` — same family as Step 1, also verified.

**Leave the Soil 0x0408 cluster (L445-457) untouched** — generic list + `add_custom_attr` is the
*correct* pattern for a cluster with no SDK helper.

## Step 4 — ErrorCode split (commit 2)

Gives the LED (and logs) a way to say "firmware never reached the air" vs "no coordinator answered."

**4a.** `firmware/shared/domain/src/errors/ErrorCode.hpp` — **append** after `Timeout,` (L31), so no
existing value renumbers (enum is `uint8_t`-backed, values may be logged):
```cpp
    Timeout,                  // generic timeout (e.g. Zigbee Mgmt_Leave_req ack)
    ZigbeeStackInitFailed,    // esp_zb platform/cluster-build/device_register failed (never on air)
    ZigbeeJoinTimeout,        // steering window expired, no parent found
```

**4b.** `ZigbeeEndDeviceAdapter.cpp` return sites:
- `ZB_RETURN_IF_FAIL` macro body (L228): `NetworkDown` → `ZigbeeStackInitFailed`. (Verified: the
  macro is used **only inside `start()`**, L349-499; `reportAttribute` returns *literal*
  `NetworkDown`/`MqttDisconnected` and is untouched.) Also remove the Step-0 `esp_err_to_name` temp line.
- `esp_zb_platform_config` failure (L302): `NetworkDown` → `ZigbeeStackInitFailed`.
- Steering timeout (L539): `WifiConnectFailed` → `ZigbeeJoinTimeout`.
- TC-mismatch (L532) and success (L560): **unchanged**.

**4c.** Update **both** stale doc comments to the new 4-outcome contract (review H1 — there are two):
- `firmware/shared/domain/src/ports/IZigbeeEndDevice.hpp` (the port contract)
- `firmware/sensor-node/lib/infrastructure/src/network/ZigbeeEndDeviceAdapter.hpp:30-31` ("Returns
  Ok or WifiConnectFailed …")

**4d.** `firmware/sensor-node/src/main.cpp` join-fail branch (L79-85) — **log only** in this commit
(keeps commit 2 compiling without the LED):
```cpp
        if (join_result == gh::domain::ErrorCode::ZigbeeTrustCenterMismatch) {
            log.error("zigbee", "TC mismatch — wiping zigbee_pair and deep sleep 1 h");
            gh::infra::ZigbeeEndDeviceAdapter::clearPairingNvs();
        } else if (join_result == gh::domain::ErrorCode::ZigbeeStackInitFailed) {
            log.error("zigbee", "stack init failed (never reached air), deep sleep 1 h");
        } else {
            log.error("zigbee", "join timeout (no parent), deep sleep 1 h");
        }
```

**4e.** Acceptance: `grep -rn WifiConnectFailed firmware/` — confirm no remaining consumer keys on
it from a Zigbee context (review M5). Add a native test for the pure mapper in Step 5's `BlinkCodes`.

## Step 5 — LED blink-code indicator (commit 3)

### New files (exact layer placement)

| File | Dir | Role |
|---|---|---|
| `Ws2812StatusLed.{hpp,cpp}` | `firmware/sensor-node/lib/infrastructure/src/platform/` | `IRgbLed` adapter via `rgbLedWrite()` (copy of the coordinator's `drivers/Ws2812StatusLed`, identical code) |
| `ArduinoBusyWait.{hpp,cpp}` | `firmware/sensor-node/lib/infrastructure/src/platform/` | tiny `IDelay` adapter — see "deterministic wait" below |
| `IDelay.hpp` | `firmware/shared/domain/src/ports/` | `struct IDelay { virtual void delayMs(uint16_t) noexcept = 0; }` |
| `BlinkCodes.hpp` | `firmware/sensor-node/lib/application/src/` | `enum class StatusCode`, `constexpr BlinkPattern patternFor(...)`, `constexpr StatusCode statusForJoinResult(ErrorCode)` |
| `StatusBlinker.{hpp,cpp}` | `firmware/sensor-node/lib/application/src/` | one-shot blocking emitter over `IRgbLed&` + `IDelay&` |

Reuse the `IRgbLed` port (`shared/domain/src/ports/IRgbLed.hpp`) as-is. **Do not** reuse the
coordinator's tick-driven `StatusLedService` — the node has no loop and RMT dies in deep sleep.

### Deterministic wait (review build-H2 — the test couldn't otherwise terminate)

`StatusBlinker` must **not** busy-wait on `IClock::nowMs()` — the repo's `FakeClock` is static and a
busy-wait would never terminate in the native test. Inject an `IDelay` port instead:
- Production `ArduinoBusyWait::delayMs()` → `::delay(ms)`. This is the **sanctioned terminal-path
  exception** to "no `delay()` > 10 ms": blinks run only immediately before a deep sleep / on a
  hardware-fault path, never in the report hot path (analogous to the allowed `SensorCycle` warmup wait).
- Native test `FakeDelay::delayMs()` → no-op, so `emit()` returns immediately and the test asserts
  the **color sequence** (the thing worth testing), not wall-clock timing.

### `BlinkCodes.hpp` — vocabulary (counts carry the signal; hue is secondary)

Review L5: red×3 vs magenta×3 are hard to tell apart on a tiny WS2812 at low brightness, so the two
failure modes differ in **count** too.

| `StatusCode` | Pattern | Meaning → operator action |
|---|---|---|
| `JoinedOk` | green ×1, 200 ms | healthy — none |
| `JoinTimeout` | red ×3 | reached air, no coordinator → open permit-join, reset node |
| `StackInitFailed` | magenta ×2 | firmware never reached air → re-flash fixed firmware |
| `TcMismatch` | amber ×1 long (800 ms) | key rejected, pairing wiped → check TC key / coordinator |
| `RailInitFailed` | red ×1 solid (1000 ms) | sensor-rail hardware fault before networking |

`statusForJoinResult(ErrorCode)`: `Ok→JoinedOk`, `ZigbeeJoinTimeout→JoinTimeout`,
`ZigbeeStackInitFailed→StackInitFailed`, `ZigbeeTrustCenterMismatch→TcMismatch`, default→`StackInitFailed`.
Colors mirror the coordinator's `Style` language (green=ok, red=fault, amber=attention).

### Config — `SensorNodeConfig.hpp` (one adjacent block after `kBatteryAdcGpio`)

```cpp
    static constexpr uint8_t kStatusLedGpio          = 8;   // onboard WS2812, strapping-safe as output
    static constexpr uint8_t kStatusLedBrightnessPct = 30;  // WS2812 is blinding at full scale
```
(`docs/hardware/reference/canonical-values.md` already records `node.status_led = GPIO8` — cross-check, no new row.)

### `main.cpp` wiring (commit 3 only)

1. **Hoist the existing `clock_`** (currently L72) to just above the LED construction and reuse it —
   do **not** add a second clock (review H4). `ArduinoClock` has no hardware in its ctor, so
   constructing it before `Wire.begin()` is safe; verify nothing between old-L72 and the new
   position uses `clock_`.
2. After `rail` (L51), before the rail-init check (L52):
   ```cpp
   static gh::infra::Ws2812StatusLed status_led{cfg::kStatusLedGpio, cfg::kStatusLedBrightnessPct};
   static gh::infra::ArduinoBusyWait waiter{};
   static gh::sensor::StatusBlinker  blinker{status_led, waiter};
   ```
3. **Rail-init fail** (insert at L53, before `sleepFor`): `blinker.emit(gh::sensor::StatusCode::RailInitFailed);`
4. **Join-fail block — full rewritten form** (review C3: emit **after** `rail.off()` so the rail is
   already powered down during the ~1 s blink; both terminal paths are reached; no fall-through):
   ```cpp
   if (join_result != gh::domain::ErrorCode::Ok) {
       if (join_result == gh::domain::ErrorCode::ZigbeeTrustCenterMismatch) {
           log.error("zigbee", "TC mismatch — wiping zigbee_pair and deep sleep 1 h");
           gh::infra::ZigbeeEndDeviceAdapter::clearPairingNvs();
       } else if (join_result == gh::domain::ErrorCode::ZigbeeStackInitFailed) {
           log.error("zigbee", "stack init failed (never reached air), deep sleep 1 h");
       } else {
           log.error("zigbee", "join timeout (no parent), deep sleep 1 h");
       }
       rail.off();
       blinker.emit(gh::sensor::statusForJoinResult(join_result));
   #ifdef GH_NODE_NO_SLEEP
       for (;;) { log.error("DIAG", "no-sleep debug: staying awake"); delay(3000); }
   #else
       sleeper.sleepFor(cfg::kFailedJoinSleepMs);
   #endif
       // unreachable
   }
   ```
   (Production `sleepFor` never returns; `GH_NODE_NO_SLEEP` is an infinite loop — no path falls
   through to the success branch.)
5. **Success** — after `cycle.runOnce()` (L95), before `sleepFor(sleep_ms)` (L100):
   `blinker.emit(gh::sensor::StatusCode::JoinedOk);`

## Tests & acceptance

- **New native test** `test/test_blink_codes/` (env `sensor-node-native`): `statusForJoinResult`
  mapping (all 5 inputs incl. default), `patternFor` values per code, and `StatusBlinker::emit()`
  sequence via a new node-local `test/fakes/FakeRgbLed.hpp` (**`namespace gh::test`**, recording the
  ordered `setColor` writes; review M3) + `FakeDelay` (no-op). Assert: `count` on-colors interleaved
  with off, final call `(0,0,0)`.
- **Unchanged & must still pass:** `test_zigbee_report_mapper` (join fix doesn't touch the report
  path), `test_sensor_cycle`, `test_sensor_registry` (`start()` is faked).
- **hwtest** `test_drivers/test_zigbee_end_device` stays a no-op placeholder (real `start()`
  registration is bench-only via `/api/nodes`); the `Ws2812StatusLed` driver is hardware-only,
  observed via the bench blink under `GH_NODE_NO_SLEEP` — call this out as a deliberate gap.
- **Revert gate (replaces the hand-curated DIAG list; review build-C1):** before the production
  flash, these must return **zero** hits —
  ```bash
  grep -rn 'DIAG' firmware/sensor-node/
  grep -rn 'GH_NODE_NO_SLEEP' firmware/sensor-node/
  grep -rn 'esp_err_to_name' firmware/sensor-node/lib/infrastructure/src/network/ZigbeeEndDeviceAdapter.cpp
  ```
  (Line-number lists go stale after the edits shift lines; grep is the reliable gate.)
- **Pre-commit (root CLAUDE [10] equivalents):** `pio run -e sensor-node` clean under `-Werror`
  + per-lib `-Wextra -Wconversion -Wshadow -fno-rtti`; `pio test -e sensor-node-native` green;
  new `StatusBlinker` use case ships with its test; new `Ws2812StatusLed` implements the `IRgbLed`
  port and stays in `infrastructure/`; no `Arduino.h`/`Wire.h`/`millis()` in `domain/`/`application/`
  (`BlinkCodes`/`StatusBlinker` use only `IRgbLed`/`IDelay` ports; `rgbLedWrite`/`delay` confined to
  the `platform/` adapters); no `new`/`malloc` (all `static` in `setup()`, patterns `constexpr`).

## Risks & rollback

- **Failure relocates after the Basic fix** → the DIAG sequence + Step-0 errno localize it; Step 3
  handles Power Config; EP2 already uses the correct typed builder.
- **Power Config `create()` default mains attrs** → verified harmless / coordinator-ignored; the §Step 3
  fallback covers a typed-adder rejection.
- **`0xFF00`/`0xF001` round-trip after join** → separate from the join: verify on the bench that the
  present-mask/report-period actually arrive (coordinator `/api/nodes` health + a `0xFF00` write).
  The report path sets `manuf_specific=1` for `attr_id ≥ 0xF000` (L605-616) while the value is
  written plain (L592); if these attrs don't round-trip, reconcile the report-side manuf flags —
  but that's a telemetry follow-up, **not** a blocker for join.
- **Rollback** is per-commit and in reverse order: commit 3 (delete the 5 new files + test + fake +
  config + main.cpp wiring — nothing depends on it), commit 2 (remove 2 enumerators + 3 return sites
  + 2 doc comments + the main.cpp log branch), commit 1 (revert Edits 1a/1b, and Step 3 if applied).
</content>
