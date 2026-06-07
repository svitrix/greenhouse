#pragma once
#include "entities/SoilSample.hpp"
#include "entities/AirSample.hpp"
#include "entities/PumpState.hpp"
#include "errors/ErrorCode.hpp"

namespace gh::domain {
struct ITelemetrySink {
    virtual ~ITelemetrySink() = default;
    [[nodiscard]] virtual ErrorCode publishSoil      (const SoilSample&) noexcept = 0;
    [[nodiscard]] virtual ErrorCode publishAir       (const AirSample&)  noexcept = 0;
    [[nodiscard]] virtual ErrorCode publishPumpState (PumpState)         noexcept = 0;
};
}
