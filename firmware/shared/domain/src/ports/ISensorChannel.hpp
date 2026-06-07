#pragma once
#include <cstdint>
#include "entities/SensorKind.hpp"
#include "entities/SensorReading.hpp"
#include "util/Result.hpp"

namespace gh::domain {

// Single port for every sensor. Replaces IAirSensor / ISoilSensor /
// IBatteryMonitor (removed at the end of Phase 1). See spec §3.1.
class ISensorChannel {
public:
    virtual ~ISensorChannel() = default;

    // Stable identity used for sensors_present_mask bit indexing.
    [[nodiscard]] virtual SensorChannelId id()       const noexcept = 0;
    [[nodiscard]] virtual SensorKind      kind()     const noexcept = 0;

    // Declared time (ms) the sensor needs after power-rail ON before its
    // first read returns valid data. SensorCycle waits max(warmupMs) over
    // Ok channels. Returns 0 if the sensor needs no warmup.
    [[nodiscard]] virtual uint32_t warmupMs() const noexcept = 0;

    // Current health. Mutated by probe() and by read() on failure.
    [[nodiscard]] virtual SensorStatus status() const noexcept = 0;

    // One-shot read. Updates status_ to Faulty on Error. Caller must ensure
    // power rail is ON and warmup elapsed.
    [[nodiscard]] virtual Result<SensorReading> read() noexcept = 0;

    // Tolerant probe used once at boot. NACK -> Absent. Read success -> Ok.
    // Updates status_ as a side effect. Caller ensures power+warmup.
    [[nodiscard]] virtual SensorStatus probe() noexcept = 0;
};

}
