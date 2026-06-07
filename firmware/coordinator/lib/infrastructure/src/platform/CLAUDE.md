# CLAUDE.md — `infrastructure/src/platform/`

> Glue adapters around ESP-IDF / Arduino-ESP32 platform primitives — clock, button, serial logger, system info. Each implements a port so the use-case layer stays hardware-agnostic.

## File map

| File                                                                | Implements                       | What it wraps                                                                                                                                          |
|---------------------------------------------------------------------|----------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------|
| [`ArduinoClock.{hpp,cpp}`](ArduinoClock.cpp)                        | `gh::domain::IClock`             | `millis()`. Returns `uint32_t` monotonic milliseconds since boot. Wraps in ~49.7 days — `IrrigationService` uses unsigned subtraction so wraparound is safe up to 49 d. |
| [`GpioButton.{hpp,cpp}`](GpioButton.cpp)                            | `gh::domain::IButton`            | `digitalRead()` + `INPUT_PULLUP`. Active-low. Used for the onboard BOOT button (GPIO9) via `kBootButtonGpio` in `AppConfig`. `holdConfirmed(hold_ms)` busy-waits up to `hold_ms` (with `delay(50)` → `vTaskDelay`, FreeRTOS-friendly). |
| [`SerialLogger.{hpp,cpp}`](SerialLogger.cpp)                        | `gh::domain::ILogger`            | ESP-IDF `esp_log` (`ESP_LOGI/W/E`) with the supplied tag. Output goes through the standard ESP-IDF `vprintf` hook, so format / colour / timestamps match Wi-Fi / Zigbee / MQTT system logs. Runtime level control: `esp_log_level_set("tag", ESP_LOG_WARN)`. |
| [`ArduinoSystemInfo.{hpp,cpp}`](ArduinoSystemInfo.cpp)              | `gh::domain::ISystemInfo`        | Fills `SystemInfo{device_id, firmware_version, ip, uptime_s, wifi_rssi_dbm}`. MAC comes from `esp_read_mac(ESP_MAC_WIFI_STA)` (eFuse-backed, works before Wi-Fi init); uptime from `esp_timer_get_time()` (`int64_t` µs, no 49 d wrap); IP / RSSI from `WiFi`. FW version is injected via ctor (`"0.4.0"` literal at the composition root today). |

## Conventions

- **One concern per file.** A logger is a logger; if you need a structured event sink, that's a different port and a different adapter.
- **No state beyond the wrapped primitive.** `ArduinoClock` has no fields. `GpioButton` has only `pin_` + `active_low_`. `ArduinoSystemInfo` holds a single `const char*` to the FW-version string. Drift from this and you've smuggled business logic into infrastructure.
- **Setup-time API choices matter.** `Serial.begin(115200)` is in `setup()`, NOT in `SerialLogger` ctor — adapters do not configure global state. (`esp_log` uses the same UART through the Arduino-ESP32 bridge, so `Serial.begin` is what hooks logs to the USB-serial bridge.)
- **Prefer ESP-IDF primitives over Arduino wrappers** where they read state before subsystems are initialised: `esp_read_mac` over `WiFi.macAddress`, `esp_timer_get_time` over `millis()/1000U`. The Arduino wrappers may silently return zeros if the underlying subsystem is not yet up.
- **Pin assignments come from `AppConfig`** (`shared/application/src/AppConfig.hpp`). Don't hard-code GPIO numbers in this directory.

## Strapping-pin caveat

`GpioButton` is used with `kBootButtonGpio = 9` — that's the onboard BOOT button **AND** a strapping pin. The DevKitM-1 already has the proper pull-up + button wiring; `INPUT_PULLUP` is safe. **Never** attach an external load to this pin — it must remain high-Z during reset. Documented in `AppConfig.hpp` next to the constant.

## ISR-safety

None of these adapters are ISR-safe.

- `SerialLogger` calls `ESP_LOGx`, which goes through `vprintf` — not for ISRs. Use `ESP_DRAM_LOGx` from interrupt context if you ever need it (and then route through a different port, not this one).
- `GpioButton::holdConfirmed` calls `delay()` — non-blocking via `vTaskDelay`, but still a task-context API.
- `ArduinoSystemInfo::snapshot` calls `WiFi.localIP()` / `WiFi.RSSI()` — Arduino-WiFi internals, not ISR-safe.

## Testing

- Each adapter has a host-side fake in [`firmware/coordinator/test/fakes/`](../../../../test/fakes/) (`FakeClock`, `FakeButton`, `FakeLogger`, `FakeSystemInfo`). Use those in `coordinator-native` tests of use cases.
- The real Arduino / ESP-IDF backed adapters compile only under `coordinator` / `coordinator-hwtest` and are exercised by integration runs on the bench.

## Adding a new platform adapter

1. Confirm there's a port for it in `shared/domain/src/ports/`. If you find yourself adding a "tiny helper" without a port — that's a smell. Stop and define the interface.
2. Create `<Name>.{hpp,cpp}` here.
3. Keep state minimal — wrap one primitive, not two.
4. Add a `Fake<Name>.hpp` to `test/fakes/` so application-layer code can be tested against it.
