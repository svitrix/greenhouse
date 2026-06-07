#pragma once
#include <cstdint>
#include "ZclIds.hpp"

namespace gh::sensor {
// Hardware pins/addresses below are mirrored in the canonical doc table —
// docs/hardware/reference/canonical-values.md (#i2c, #sensor-node-gpio,
// #i2c-devices). This header is the runtime truth.
struct SensorNodeConfig {
    static constexpr uint8_t  kI2cSdaPin           = 6;
    static constexpr uint8_t  kI2cSclPin           = 7;
    static constexpr uint32_t kI2cFrequencyHz      = 100'000;
    // Settle time after the p-MOSFET gate goes LOW (sensors powered). I²C
    // parts need rail stable before the first transaction in probeAll().
    static constexpr uint32_t kSensorRailSettleMs    = 400;
    // GPIO4 — strapping pin. Schematic MUST have external 100 kΩ pull-up
    // on the gate line; see SensorPowerGate.hpp for the full rationale.
    static constexpr uint8_t  kSensorPowerGateGpio = 4;
    // GPIO0 = ADC1_CH0 on ESP32-C6.  Not strapping, LP-capable; safe for
    // a divider midpoint.  See root CLAUDE.md §0.1.
    static constexpr uint8_t  kBatteryAdcGpio      = 0;
    // GPIO8 — onboard WS2812 status LED (strapping pin, boot-safe as output).
    // No gpio_hold needed: the LED is dark during deep sleep by design — each
    // blink code is emitted once on the terminal pre-sleep path.
    static constexpr uint8_t  kStatusLedGpio          = 8;
    // WS2812 is blinding at full scale; cap perceived brightness.
    static constexpr uint8_t  kStatusLedBrightnessPct = 30;
    static constexpr uint8_t  kAm2315cAddress      = 0x38;
    static constexpr uint8_t  kChirpAddress        = 0x20;

    static constexpr uint32_t kBatteryDividerR1Ohm = 100'000;
    static constexpr uint32_t kBatteryDividerR2Ohm = 100'000;

    static constexpr uint32_t kZbSteeringTimeoutMs = 60'000;
    // Sleepy ED tx latency = parent-poll + send + ack; under noisy 2.4 GHz
    // a healthy cycle can land at 2 s.  1.5 s timeout was sporadically
    // missing real acks.
    static constexpr uint32_t kZbTxTimeoutMs       = 3'000;

    // Used by main.cpp on fatal init / pair failures. 1 hour deep sleep
    // keeps the device economical when (e.g.) the coordinator isn't running.
    static constexpr uint32_t kFailedJoinSleepMs   = 60UL * 60UL * 1000UL;

    // Mirror of protocol-layer default so callers can see the value without
    // having to chase ZclIds.hpp.
    static constexpr uint32_t kDefaultReportPeriodS =
        gh::protocol::kReportPeriodDefaultS;
};
}
