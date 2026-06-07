#pragma once
#include "ports/ISensorChannel.hpp"

namespace gh::test {

class FakeSensorChannel final : public gh::domain::ISensorChannel {
public:
    gh::domain::SensorChannelId id_{0};
    gh::domain::SensorKind      kind_{gh::domain::SensorKind::Air};
    uint32_t                    warmup_ms_{0};
    gh::domain::SensorStatus    status_{gh::domain::SensorStatus::Unprobed};
    gh::domain::SensorStatus    next_probe_status{gh::domain::SensorStatus::Ok};
    gh::domain::Result<gh::domain::SensorReading> next_read{
        gh::domain::ErrorCode::Ok, {}};
    int read_calls  = 0;
    int probe_calls = 0;

    [[nodiscard]] gh::domain::SensorChannelId id()       const noexcept override { return id_; }
    [[nodiscard]] gh::domain::SensorKind      kind()     const noexcept override { return kind_; }
    [[nodiscard]] uint32_t                    warmupMs() const noexcept override { return warmup_ms_; }
    [[nodiscard]] gh::domain::SensorStatus    status()   const noexcept override { return status_; }

    [[nodiscard]] gh::domain::Result<gh::domain::SensorReading> read() noexcept override {
        ++read_calls;
        if (!next_read.ok()) status_ = gh::domain::SensorStatus::Faulty;
        return next_read;
    }

    [[nodiscard]] gh::domain::SensorStatus probe() noexcept override {
        ++probe_calls;
        status_ = next_probe_status;
        return status_;
    }
};

}
