// Arduino-ESP32 headers (WString.h, NetworkEvents.h) contain benign
// -Wconversion / -Wshadow issues in SDK code we cannot fix upstream.
// Suppress only around these SDK includes; our own headers follow after.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#ifdef GH_NODE_NO_SLEEP
#include <esp_task_wdt.h>
#endif
#pragma GCC diagnostic pop
#include "SensorCycle.hpp"
#include "SensorRegistry.hpp"
#include "SensorNodeConfig.hpp"
#include "BlinkCodes.hpp"
#include "StatusBlinker.hpp"
#include "drivers/AM2315CSensor.hpp"
#include "drivers/ChirpSoilSensor.hpp"
#include "platform/BatteryMonitor.hpp"
#include "platform/DeepSleepClock.hpp"
#include "platform/GpioPowerRail.hpp"
#include "platform/ArduinoClock.hpp"
#include "platform/SerialLogger.hpp"
#include "platform/Ws2812StatusLed.hpp"
#include "platform/ArduinoBusyWait.hpp"
#include "network/ZigbeeEndDeviceAdapter.hpp"
#include "network/ZigbeeReportMapper.hpp"
#include "network/ChannelMappings.hpp"
#include "ZigbeeNetwork.hpp"
#include "secrets.hpp"

using cfg = gh::sensor::SensorNodeConfig;

void setup() {
    Serial.begin(115200);
#ifdef GH_NODE_NO_SLEEP
    delay(5000);  // let USB-CDC enumerate before Zigbee init (diagnostic build only)
#endif
    static gh::infra::SerialLogger    log{};
    log.info("sensor-node", "boot");
    // Operator-facing TC-key sanity check — both firmwares log the first 4
    // bytes so a mismatched secrets.hpp is visible without leaking the key.
    char tc_prefix[16] = {0};
    snprintf(tc_prefix, sizeof(tc_prefix), "tc-key %02x%02x%02x%02x",
             gh::protocol::kZigbeeTcLinkKey[0],
             gh::protocol::kZigbeeTcLinkKey[1],
             gh::protocol::kZigbeeTcLinkKey[2],
             gh::protocol::kZigbeeTcLinkKey[3]);
    log.info("zb", tc_prefix);

    // Battery device — kill every radio we don't use BEFORE bringing Zigbee up.
    // Arduino-ESP32 brings the Wi-Fi netif up by default; that costs tens of mA
    // until something stops it, and on ESP32-C6 it shares the RF front-end with
    // native 802.15.4 → leaving Wi-Fi up flakes Zigbee joins.
    WiFi.mode(WIFI_OFF);
    btStop();

    static gh::infra::DeepSleepClock  sleeper;
    static gh::infra::GpioPowerRail   rail{cfg::kSensorPowerGateGpio};
    // Status LED + one-shot blinker. Construction touches no hardware until the
    // first emit(); the LED is the node's only production signal (no USB logs).
    static gh::infra::Ws2812StatusLed status_led{
        cfg::kStatusLedGpio, cfg::kStatusLedBrightnessPct};
    static gh::infra::ArduinoBusyWait waiter;
    static gh::sensor::StatusBlinker  blinker{status_led, waiter};
    if (rail.init() != gh::domain::ErrorCode::Ok) {
        log.error("rail", "init failed; deep sleep 1 h");
        blinker.emit(gh::sensor::StatusCode::RailInitFailed);
        sleeper.sleepFor(cfg::kFailedJoinSleepMs);
        return;
    }

    Wire.begin(cfg::kI2cSdaPin, cfg::kI2cSclPin, cfg::kI2cFrequencyHz);
    static gh::infra::AM2315CSensor   air{Wire, cfg::kAm2315cAddress};
    static gh::infra::ChirpSoilSensor soil{Wire, cfg::kChirpAddress};
    static gh::infra::BatteryMonitor  battery{
        cfg::kBatteryAdcGpio,
        cfg::kBatteryDividerR1Ohm,
        cfg::kBatteryDividerR2Ohm};

    static gh::app::SensorRegistry registry;
    registry.add(air);
    registry.add(soil);
    registry.add(battery);
    const size_t probe_ok = registry.probeAll(rail, log);
#if defined(GH_SENSOR_DIAG_LOG) || defined(GH_NODE_NO_SLEEP)
    {
        char probe_buf[48] = {0};
        snprintf(probe_buf, sizeof(probe_buf), "probe ok=%u mask=0x%02X",
                 static_cast<unsigned>(probe_ok),
                 static_cast<unsigned>(registry.presentMask() & 0xFFu));
        log.info("registry", probe_buf);
        log.error("DIAG", probe_buf);
    }
#endif

    static gh::infra::ArduinoClock            clock_;
    static gh::infra::ZigbeeEndDeviceAdapter  zb{log, status_led};
    static gh::infra::ZigbeeReportMapper      mapper{zb, gh::infra::kChannelMappings};
    static gh::sensor::SensorCycle cycle{
        registry, rail, mapper, zb, sleeper, clock_, log, cfg::kZbTxTimeoutMs};

    const auto join_result = zb.start(cfg::kZbSteeringTimeoutMs);
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
        // Rail off first, then blink (rail powered down during the ~1 s pattern),
        // then the terminal deep sleep. Distinct code per failure mode.
        blinker.emit(gh::sensor::statusForJoinResult(join_result));
#ifdef GH_NODE_NO_SLEEP
        esp_task_wdt_delete(nullptr);  // unsubscribe from TWDT (diagnostic build only)
        for (;;) {
            char hb[128];
            snprintf(hb, sizeof(hb), "zb_err: %s",
                     gh::infra::ZigbeeEndDeviceAdapter::initErrContext());
            log.error("DIAG", hb);
            delay(3000);
        }
#else
        sleeper.sleepFor(cfg::kFailedJoinSleepMs);
#endif
        // unreachable
    }

    const uint32_t sleep_ms = cycle.runOnce();
    blinker.emit(gh::sensor::StatusCode::JoinedOk);
#ifdef GH_NODE_NO_SLEEP
    (void)sleep_ms;
    esp_task_wdt_delete(nullptr);  // unsubscribe from TWDT (diagnostic build only)
    for (;;) { log.info("DIAG", "no-sleep debug: cycle done, awake"); delay(5000); }
#else
    sleeper.sleepFor(sleep_ms);
#endif
    // unreachable
}

void loop() {
    // setup() never returns from sleepFor(); loop() is unreachable.
}
