#include "BatteryMonitor.hpp"
// Arduino-ESP32 headers (WString.h) have benign -Wconversion issues we
// cannot fix upstream. Suppress only around the SDK include.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include <Arduino.h>
#pragma GCC diagnostic pop

namespace gh::infra {

namespace {
constexpr uint16_t kSamples         = 8;  // average to reduce ADC noise
constexpr uint8_t  kMaxAdc1ChGpioC6 = 6;  // ESP32-C6: ADC1 channels 0..6 only
}

BatteryMonitor::BatteryMonitor(uint8_t  adc_gpio,
                                uint32_t r_top_ohm,
                                uint32_t r_bottom_ohm) noexcept
    : adc_gpio_(adc_gpio), r_top_ohm_(r_top_ohm), r_bottom_ohm_(r_bottom_ohm) {}

gh::domain::Result<gh::domain::BatteryReading>
BatteryMonitor::readBattery() noexcept {
    using R = gh::domain::Result<gh::domain::BatteryReading>;

    // ESP32-C6 has ADC1 only (no ADC2).  analogReadMilliVolts returns 0 for
    // a GPIO that isn't ADC1-capable — silently writing "0 V → 0 % battery"
    // would falsely alarm the coordinator.  Refuse early.
    if (adc_gpio_ > kMaxAdc1ChGpioC6 || r_bottom_ohm_ == 0U) {
        return R::failure(gh::domain::ErrorCode::SensorNotReady);
    }

    // Explicit INPUT before the first ADC sample — arduino-esp32 logs
    // "Pin is not configured as analog channel" otherwise on ESP32-C6.
    static bool s_pin_configured = false;
    if (!s_pin_configured) {
        pinMode(adc_gpio_, INPUT);
        s_pin_configured = true;
    }

    // Explicitly set 11 dB attenuation for the 0..2.1 V post-divider range.
    // The arduino-esp32 default is also 11 dB, but it must not be implicit —
    // a future board-init change could silently change the scale.
    static bool s_attenuation_set = false;
    if (!s_attenuation_set) {
        analogSetPinAttenuation(adc_gpio_, ADC_11db);
        s_attenuation_set = true;
    }

    // arduino-esp32 analogReadMilliVolts uses calibrated multi-point ADC.
    uint32_t sum_mv = 0;
    for (uint16_t i = 0; i < kSamples; ++i) {
        sum_mv += analogReadMilliVolts(adc_gpio_);
    }
    const uint32_t avg_adc_mv = sum_mv / kSamples;

    // All-zero samples are an unrecoverable read failure (bad pin, broken
    // divider, gate off) — let the cycle log + skip instead of reporting 0 V.
    if (avg_adc_mv == 0U) {
        return R::failure(gh::domain::ErrorCode::SensorNotReady);
    }

    // Vbat = Vadc * (r_top + r_bottom) / r_bottom.
    const uint64_t numerator = static_cast<uint64_t>(avg_adc_mv) *
                                static_cast<uint64_t>(r_top_ohm_ + r_bottom_ohm_);
    const uint16_t vbat_mv = static_cast<uint16_t>(numerator / r_bottom_ohm_);

    gh::domain::BatteryReading r{};
    r.voltage_mv             = vbat_mv;
    r.state_of_charge_pct    = voltageToSocPct(vbat_mv);
    return R::success(r);
}

// Piecewise-linear LiPo curve (4.20V=100%, 3.70V=50%, 3.40V=5%, 3.20V=0%).
uint8_t BatteryMonitor::voltageToSocPct(uint16_t mv) noexcept {
    if (mv >= 4200) return 100;
    if (mv >= 3700) {
        // 3700..4200 → 50..100 (linear)
        return static_cast<uint8_t>(50 + (static_cast<uint32_t>(mv - 3700) * 50U) / 500U);
    }
    if (mv >= 3400) {
        // 3400..3700 → 5..50 (linear)
        return static_cast<uint8_t>(5 + (static_cast<uint32_t>(mv - 3400) * 45U) / 300U);
    }
    if (mv >= 3200) {
        // 3200..3400 → 0..5 (linear)
        return static_cast<uint8_t>((static_cast<uint32_t>(mv - 3200) * 5U) / 200U);
    }
    return 0;
}

gh::domain::Result<gh::domain::SensorReading>
BatteryMonitor::read() noexcept {
    using R = gh::domain::Result<gh::domain::SensorReading>;
    auto b = readBattery();
    if (!b.ok()) {
        status_ = gh::domain::SensorStatus::Faulty;
        return R::failure(b.err);
    }
    status_ = gh::domain::SensorStatus::Ok;
    gh::domain::SensorReading r{};
    r.id             = gh::domain::SensorChannelId{gh::domain::kSensorChannelIdBattery};
    r.kind           = gh::domain::SensorKind::Battery;
    r.read_at_ms     = 0;
    r.values.battery = b.value;
    return R::success(r);
}

gh::domain::SensorStatus BatteryMonitor::probe() noexcept {
    // ADC channel — always present in hardware. A read failure here means
    // misconfigured pin or out-of-range divider. Treat as Faulty, not Absent.
    auto b = readBattery();
    status_ = b.ok() ? gh::domain::SensorStatus::Ok : gh::domain::SensorStatus::Faulty;
    return status_;
}

}
