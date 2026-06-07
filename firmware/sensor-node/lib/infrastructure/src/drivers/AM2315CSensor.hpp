#pragma once
#include <Wire.h>
#include "ports/ISensorChannel.hpp"

namespace gh::infra {

class AM2315CSensor final : public gh::domain::ISensorChannel {
public:
    explicit AM2315CSensor(TwoWire& bus, uint8_t address = 0x38) noexcept;

    [[nodiscard]] gh::domain::ErrorCode init() noexcept;

    // ISensorChannel overrides
    [[nodiscard]] gh::domain::SensorChannelId id() const noexcept override
        { return gh::domain::SensorChannelId{gh::domain::kSensorChannelIdAir}; }
    [[nodiscard]] gh::domain::SensorKind      kind() const noexcept override
        { return gh::domain::SensorKind::Air; }
    [[nodiscard]] uint32_t                    warmupMs() const noexcept override
        { return kWarmupMs; }
    [[nodiscard]] gh::domain::SensorStatus    status() const noexcept override
        { return status_; }
    [[nodiscard]] gh::domain::Result<gh::domain::SensorReading>
        read() noexcept override;
    [[nodiscard]] gh::domain::SensorStatus probe() noexcept override;

    // Hardware-test helper: returns the raw AirSample. Production code uses read().
    [[nodiscard]] gh::domain::Result<gh::domain::AirSample>
        readAir() noexcept;

    static constexpr uint32_t kWarmupMs = 80;  // AM2315C datasheet: ≥100 ms;
                                               // we share the rail with Chirp's 800 ms anyway.

private:
    TwoWire& bus_;
    uint8_t  address_;
    bool     initialised_ = false;
    gh::domain::SensorStatus status_ = gh::domain::SensorStatus::Unprobed;

    [[nodiscard]] gh::domain::ErrorCode resetQuirkIfNeeded_() noexcept;
    [[nodiscard]] gh::domain::ErrorCode resetOneRegister_(uint8_t reg) noexcept;
    [[nodiscard]] gh::domain::ErrorCode triggerMeasurement_() noexcept;
    [[nodiscard]] gh::domain::Result<uint8_t> readStatus_() noexcept;
    [[nodiscard]] gh::domain::ErrorCode waitNotBusy_(uint32_t timeout_ms) noexcept;
    [[nodiscard]] gh::domain::ErrorCode readBytes_(uint8_t* buf, uint8_t len) noexcept;

    [[nodiscard]] static uint8_t crc8_(const uint8_t* data, uint8_t len) noexcept;
};
}
