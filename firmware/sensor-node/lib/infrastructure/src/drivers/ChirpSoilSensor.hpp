#pragma once
#include <Wire.h>
#include "ports/ISensorChannel.hpp"

namespace gh::infra {

class ChirpSoilSensor final : public gh::domain::ISensorChannel {
public:
    ChirpSoilSensor(TwoWire& bus, uint8_t address,
                    gh::domain::SensorChannelId channel_id =
                        gh::domain::SensorChannelId{gh::domain::kSensorChannelIdSoil1}) noexcept;

    [[nodiscard]] gh::domain::ErrorCode init() noexcept;

    // ISensorChannel identity overrides
    [[nodiscard]] gh::domain::SensorChannelId id() const noexcept override
        { return channel_id_; }
    [[nodiscard]] gh::domain::SensorKind      kind() const noexcept override
        { return gh::domain::SensorKind::Soil; }
    [[nodiscard]] uint32_t                    warmupMs() const noexcept override
        { return kWarmupMs; }
    [[nodiscard]] gh::domain::SensorStatus    status() const noexcept override
        { return status_; }
    [[nodiscard]] gh::domain::Result<gh::domain::SensorReading>
        read() noexcept override;
    [[nodiscard]] gh::domain::SensorStatus probe() noexcept override;

    // Hardware-test helper: returns the raw SoilSample. Production code uses read().
    [[nodiscard]] gh::domain::Result<gh::domain::SoilSample>
        readSoil() noexcept;

    // 1000 ms — matches kPostResetWarmupMs in init(). After SensorRegistry::probeAll
    // powers the rail OFF/ON, the Chirp hard-boots from cold but ChirpSoilSensor::read()
    // skips init() (initialised_ is still true in RAM). 1000 ms covers the cold boot.
    static constexpr uint32_t kWarmupMs = 1000;

private:
    TwoWire& bus_;
    uint8_t  address_;
    bool     initialised_ = false;
    gh::domain::SensorChannelId channel_id_;
    gh::domain::SensorStatus    status_ = gh::domain::SensorStatus::Unprobed;

    [[nodiscard]] gh::domain::ErrorCode waitNotBusy_() noexcept;
    [[nodiscard]] gh::domain::ErrorCode writeCommand_(uint8_t cmd) noexcept;
    [[nodiscard]] gh::domain::Result<uint8_t>  readByte_(uint8_t cmd) noexcept;
    [[nodiscard]] gh::domain::Result<uint16_t> writeRegAndRead16_(uint8_t cmd) noexcept;

    [[nodiscard]] static gh::domain::ErrorCode endTxToError_(uint8_t code) noexcept;
};
}
