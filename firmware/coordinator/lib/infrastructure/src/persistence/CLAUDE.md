# CLAUDE.md — `infrastructure/src/persistence/`

> NVS-backed config stores. Each adapter wraps an ESP-IDF `Preferences` namespace and implements a port from `shared/domain/src/ports/`. **Plain-text storage** — documented MVP trade-off in root [`CLAUDE.md`](../../../../../../CLAUDE.md) §9.5.

## File map

| File                                                                                   | Implements                              | NVS namespace      | Schema (keys)                                                                                                  |
|----------------------------------------------------------------------------------------|-----------------------------------------|--------------------|----------------------------------------------------------------------------------------------------------------|
| [`NvsWifiCredsStore.{hpp,cpp}`](NvsWifiCredsStore.cpp)                                 | `gh::domain::IWifiCredsStore`           | `wifi`             | `ssid` (bytes), `password` (bytes)                                                                              |
| [`NvsMqttCredsStore.{hpp,cpp}`](NvsMqttCredsStore.cpp)                                 | `gh::domain::IMqttCredsStore`           | `mqtt`             | `host`, `port` (u16), `user`, `password`, `client_id`                                                           |
| [`NvsSoilCalibrationStore.{hpp,cpp}`](NvsSoilCalibrationStore.cpp)                     | `gh::domain::ISoilCalibrationStore`     | `soil_calib`       | `raw_dry` (u16), `raw_wet` (u16)                                                                                 |
| [`NvsAdminCredsStore.{hpp,cpp}`](NvsAdminCredsStore.cpp)                               | `gh::domain::IAdminCredsStore`          | `admin`            | `user`, `pw_hash` (32-byte PBKDF2-HMAC-SHA256), `salt` (16 bytes), `iter` (u32 — PBKDF2 iterations; absent / 0 = legacy single-SHA record, upgraded in place on next login) |
| [`NvsZigbeeNetStore.{hpp,cpp}`](NvsZigbeeNetStore.cpp)                                 | `gh::domain::IZigbeeNetStore`           | `zigbee_net`       | `extpanid` (8 bytes — randomised on first boot, persisted)                                                       |
| [`NvsAnalyticsConfigStore.{hpp,cpp}`](NvsAnalyticsConfigStore.cpp)                     | `gh::domain::IAnalyticsConfigStore`     | `analytics`        | `url`, `key` (api_key from pairing claim), `period_s` (u32, default 900), `insecure` (bool — TLS bypass)        |
| [`NvsAutoWaterConfigStore.{hpp,cpp}`](NvsAutoWaterConfigStore.cpp)                     | `gh::domain::IAutoWaterConfigStore`     | `auto_water`       | `enabled` (bool), `threshold_pct` (u8), `min_fresh` (u8 — minimum quorum of fresh soil nodes), `dwell_s` (u32)  |
| [`NvsNodeAliasStore.{hpp,cpp}`](NvsNodeAliasStore.cpp)                                 | `gh::domain::INodeAliasStore`           | `nodes_alias`      | One key per IEEE long address (16-hex, colon-free) → UTF-8 alias up to 23 bytes                                  |
| [`NvsWifiFailCounterStore.{hpp,cpp}`](NvsWifiFailCounterStore.cpp)                     | `gh::domain::IWifiFailCounterStore`     | `wifi_fail`        | `count` (u32) — incremented on each STA-connect failure; reset on success                                        |
| [`NvsProvisioningFlagStore.{hpp,cpp}`](NvsProvisioningFlagStore.cpp)                   | `gh::domain::IProvisioningFlagStore`    | `prov_flag`        | `pending` (bool) — set when captive portal saved creds, cleared after first successful STA connect              |
| [`NvsLastConnectErrorStore.{hpp,cpp}`](NvsLastConnectErrorStore.cpp)                   | `gh::domain::ILastConnectErrorStore`    | `last_err`         | `code` (u8), `ts` (u32) — most recent failed-connect error reported via the SPA                                  |
| [`LittleFsTelemetryQueue.{hpp,cpp}`](LittleFsTelemetryQueue.cpp)                       | `gh::domain::ITelemetryQueue`           | LittleFS partition | D-1 outbound queue. Backed by a wear-aware ring on the `littlefs` partition (not NVS). Flushed by `AnalyticsUploader`. |

Plus the one-shot upgrade flag namespace owned by `V1MqttPurge`:

| Namespace   | Owner                                        | Keys                                                                                |
|-------------|----------------------------------------------|-------------------------------------------------------------------------------------|
| `nvs_flags` | `application/telemetry/V1MqttPurge`          | `mqtt_purge_v1` (bool — set once the v1 retained-topic cleanup pass has finished). Room for further one-shot upgrade flags. |

## Conventions

- **One file per namespace.** Do not multiplex namespaces inside a single store — keep the 1:1 mapping.
- **`begin()` returns `ErrorCode`** — never throw, never `abort()`. Caller logs and continues with defaults from `AppConfig`.
- **Reads return `Result<T>` or `std::optional<T>`** — empty key is not an error, it's "no value yet".
- **No `String` (Arduino) in stored structs.** Use `char[N]` value-types (e.g. `WifiCreds.ssid[33]`). The fixed sizes are part of the schema; bumping them is a migration.
- **Schema migrations:** if you change a key's size or rename it, bump the namespace name (`mqtt` → `mqtt_v2`) and ship a one-shot reader for the old one. Do NOT silently change layout in place.
- **Blitted structs carry a version byte.** `WifiCreds` / `MqttCreds` / `SoilCalibration` are stored via raw `putBytes`/`getBytes` under key `"v1"`. Each has `uint8_t schema_version` as its **first** member, stamped on `save()` and verified on `load()` (`SensorVersionMismatch` on mismatch). A legacy record written before the version byte existed is a different `sizeof` and is rejected as `ConfigNotFound` (re-provision / re-write). `char[]` fields are force-NUL-terminated (`normalizeForStorage()`) on load and `valid()` requires a NUL within bounds — this prevents an over-read in e.g. `WiFi.begin(ssid)` from a corrupt record.
- **Namespace isolation (E6).** `NvsWifiFailCounterStore` → `wifi_fail`, `NvsLastConnectErrorStore` → `last_err`, `NvsProvisioningFlagStore` → `prov_flag` — each owns its namespace per the file map above (they previously multiplexed `wifi`/`system`). Migration: the old values are intentionally **not** carried over — fail-counter (transient boot budget), last-connect-error (diagnostic), and provisioning-pending (defaults to "not pending") all read back safe defaults on upgrade, so a one-time reset is harmless and avoids a fragile cross-namespace reader.

## Security (do not skip)

- Everything stored here is **plain text**. Anyone with USB access can read flash:
  ```
  esptool.py --port /dev/ttyACM0 read_flash 0x9000 0x6000 nvs_dump.bin
  ```
  and recover passwords via `nvs_partition_gen.py`.
- This is acceptable for a DIY greenhouse in a trusted location (root §9.5). For production-grade you'd enable NVS encryption + Secure Boot v2 + flash encryption — irreversible (e-fuse burn) and explicitly out of scope.
- **Never** log a stored password via `Serial.print`, not even in debug.
- The `/api/config` REST endpoint (Phase C) returns `password_set: true/false` — **never the password itself**. Same rule applies to MQTT discovery payloads.

## Testing

- Native env can't talk to NVS — these adapters live in the `coordinator-hwtest` env only. Tests are in [`firmware/coordinator/test/test_drivers/test_nvs_*/`](../../../../test/test_drivers/).
- For host-side use cases that depend on these stores, inject the corresponding `Fake*Store` from [`firmware/coordinator/test/fakes/`](../../../../test/fakes/).

## Adding a new NVS-backed config

1. Define the value-object (e.g. `IrrigationConfig`) in `shared/domain/src/entities/` — POD only, fixed-size.
2. Add the port interface `IIrrigationConfigStore` in `shared/domain/src/ports/`.
3. Implement `NvsIrrigationConfigStore.{hpp,cpp}` here with a NEW namespace (do not piggy-back on `wifi` etc.).
4. Add a `FakeIrrigationConfigStore.hpp` in `test/fakes/` for host-side use-case tests.
5. Add an hwtest in `test/test_drivers/test_nvs_irrigation_config_store/` (round-trip: write → close → reopen → read).
