#pragma once
#include <cstdint>
#include "entities/AirSample.hpp"
#include "entities/SoilSample.hpp"
#include "util/Result.hpp"

namespace gh::domain {

// Fresh telemetry cache from sensor-nodes. Coordinator-only.
//
// updateAir/updateSoil are called from the Zigbee callback context.
// latestAir/latestSoil are called from the application tick.
// Implementation must be thread-safe (Zigbee callback ≠ main task).
struct ISensorCache {
    virtual ~ISensorCache() = default;

    // Returns success(sample) if a sample exists and is no older than max_age_ms,
    // otherwise failure(ErrorCode::SensorNotReady).
    [[nodiscard]] virtual Result<AirSample>
    latestAir(uint32_t now_ms, uint32_t max_age_ms) const noexcept = 0;

    [[nodiscard]] virtual Result<SoilSample>
    latestSoil(uint32_t now_ms, uint32_t max_age_ms) const noexcept = 0;

    virtual void updateAir (const AirSample&,  uint32_t now_ms) noexcept = 0;
    virtual void updateSoil(const SoilSample&, uint32_t now_ms) noexcept = 0;
};

}
