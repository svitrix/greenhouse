# ADR: Wi-Fi Provisioning and Hub Pairing

> **Status:** Accepted — implemented in:
> - `firmware/coordinator/lib/application/src/WifiProvisioner.cpp`
> - `firmware/coordinator/lib/infrastructure/src/network/EspPairingClient.hpp`
> - `firmware/coordinator/lib/presentation/src/ProvisioningWebServer.cpp`
> - SPA route `#/setup` and `#/pair`

These are two distinct flows that happen at different times.

---

## Part 1 — Wi-Fi Provisioning (config mode trigger)

### D1 — What triggers config mode

**Five-trigger precedence chain** (evaluated in order at every boot):

| Priority | Trigger | Rationale |
|---|---|---|
| 1 | NVS `force_provisioning` flag set | CLI / firmware-requested reset; cleared after entering AP mode |
| 2 | BOOT button held ≥ 3 s (GPIO9) | Hardware recovery independent of NVS state or network |
| 3 | Consecutive failed STA boots ≥ threshold | Detects credential rot (SSID renamed, password changed) |
| 4 | NVS `wifi` namespace empty | First boot after flash |
| 5 | STA connection fails after 3 retries × 30 s | Real-time network failure |

On any trigger → Soft-AP (`greenhouse-XXXX`, WPA2, last 4 MAC octets as passphrase)
+ captive DNS + ESPAsyncWebServer provisioning UI.

**Why 3-second hold threshold for BOOT button:**
Zero false-positives from accidental taps during normal operation. Button is GPIO9
(the DevKit BOOT button) — no additional hardware needed.

**Why fail counter, not just retry-on-STA-fail:**
Without a counter, a temporarily down router causes a provisioning loop on every
reboot. The counter requires repeated failures across multiple boots before entering
AP mode, distinguishing transient outage from broken credentials.

---

### D2 — HTTP framework

**Alternatives:**
- **A. `WebServer.h` (sync, blocking).** ~12 KB flash. Sufficient for config form.
- **B. `ESPAsyncWebServer`.** Async, ~50 KB flash. Supports chunked responses,
  WebSocket, SSE — required for the operational REST API and SSE dashboard.

**Choice: B.**

Changing the framework later means rewriting all routes. The operational mode REST API
(13 endpoints + SSE) requires async. Taking 50 KB flash at <7% total flash usage is
not a constraint.

---

### D3 — Provisioning wizard scope

**One-pass wizard covers all credentials:**
- Wi-Fi SSID + password + hostname
- MQTT host / port / username / password / client_id
- Soil calibration `raw_dry` / `raw_wet`
- Admin username + password (hashed as SHA-256(salt + password) in NVS)
- Analytics backend URL + API key (optional, for D-1 hub ingestion)

**Why all at once:** `secrets.h` becomes unused after first provisioning. One screen →
device is fully operational. Splitting into multiple wizards creates partial-state
failure modes.

---

### D4 — NVS storage strategy

**Separate narrow port per credential type:**
`IWifiCredsStore`, `IMqttCredsStore`, `ISoilCalibrationStore`, `IAdminCredsStore`,
`IAnalyticsConfigStore` — each with its own NVS namespace.

**Why not a generic `IConfigStore<T>` template:**
Constitution II (`-fno-rtti`) makes templates inflate flash. Type-safety: you cannot
accidentally write WiFi creds into the MQTT namespace. Each namespace is independently
readable/writeable without locking unrelated config.

---

### D5 — Admin password hashing

Admin credentials are stored as `SHA-256(16-byte-random-salt + password)`. The salt
is generated once per provisioning and stored alongside the hash. Plaintext password
is never persisted.

**Why not bcrypt/Argon2:** these require significant RAM and compute time, incompatible
with ESP32-C6's 512 KB SRAM and embedded use. SHA-256 with a random salt is sufficient
for a LAN-only access control with no remote attack surface (constitution §9.5 threat
model: "a random guest with a laptop").

---

## Part 2 — Hub Pairing (D-2, separate flow)

### D6 — Hub pairing as a separate concern from Wi-Fi provisioning

Hub pairing (getting a device API key from the analytics hub) is initiated from the
SPA route `#/pair` **after** Wi-Fi provisioning is complete and the coordinator is
operational. It is not part of the provisioning wizard.

**Why separate:**
- Pairing requires the coordinator to be online (needs STA connection to reach hub).
- Hub pairing is **optional** — analytics (D-1) is gated by `analytics/api_key` NVS
  key. A coordinator without an API key still works fully for local MQTT/HA.
- Mixing pairing into the captive portal form would make provisioning fail in offline
  environments (no hub reachable).

### D7 — Hue-style 6-digit claim code

The `#/pair` route lets the operator enter a 6-digit claim code opened on the hub
(`POST /api/pairing/open`). The coordinator's `EspPairingClient` posts
`{claim_code, device_id, mac, fw_version, profile_id}` to the hub's
`POST /api/pairing/claim` endpoint and receives an API key.

**Why this UX:** Mirrors Philips Hue onboarding — operator opens a time-limited window
on the hub (via admin token) and enters the code on the device. No secrets travel over
QR codes or unencrypted channels. The claim code is short-lived (default 5 min TTL).

The hub uses `SELECT … FOR UPDATE` to serialize concurrent claim attempts — only one
coordinator can claim the same code (see `constitution-hub.md` §VI).
