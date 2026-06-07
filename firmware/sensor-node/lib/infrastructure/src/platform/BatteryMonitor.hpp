#pragma once
#include <cstdint>
#include "ports/ISensorChannel.hpp"

namespace gh::infra {

class BatteryMonitor final : public gh::domain::ISensorChannel {
public:
    // adc_gpio: must be ADC1_CHx-capable (GPIO0..GPIO6 on ESP32-C6).
    // r_top / r_bottom: divider resistors in ohm. Vbat = Vadc * (r_top + r_bottom) / r_bottom.
    BatteryMonitor(uint8_t  adc_gpio,
                   uint32_t r_top_ohm,
                   uint32_t r_bottom_ohm) noexcept;

    // ISensorChannel identity overrides
    [[nodiscard]] gh::domain::SensorChannelId id() const noexcept override
        { return gh::domain::SensorChannelId{gh::domain::kSensorChannelIdBattery}; }
    [[nodiscard]] gh::domain::SensorKind      kind() const noexcept override
        { return gh::domain::SensorKind::Battery; }
    [[nodiscard]] uint32_t                    warmupMs() const noexcept override
        { return 0; }
    [[nodiscard]] gh::domain::SensorStatus    status() const noexcept override
        { return status_; }
    [[nodiscard]] gh::domain::Result<gh::domain::SensorReading>
        read() noexcept override;
    [[nodiscard]] gh::domain::SensorStatus probe() noexcept override;

    // Hardware-test helper: returns the raw BatteryReading. Production code uses read().
    [[nodiscard]] gh::domain::Result<gh::domain::BatteryReading>
        readBattery() noexcept;

private:
    uint8_t  adc_gpio_;
    uint32_t r_top_ohm_;
    uint32_t r_bottom_ohm_;
    gh::domain::SensorStatus status_ = gh::domain::SensorStatus::Unprobed;

    static uint8_t voltageToSocPct(uint16_t mv) noexcept;
};

}
