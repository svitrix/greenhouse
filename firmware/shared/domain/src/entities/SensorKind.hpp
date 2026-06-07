#pragma once
#include <cstdint>

namespace gh::domain {

enum class SensorKind : uint8_t {
    Air     = 0,
    Soil    = 1,
    Battery = 2,
    Light   = 3,
    Co2     = 4,
};

enum class SensorStatus : uint8_t {
    Unprobed = 0,  // boot default; probe not yet attempted
    Ok       = 1,  // last probe/read succeeded
    Absent   = 2,  // probe returned I2C NACK / no ACK
    Faulty   = 3,  // probe ok previously, but a subsequent read failed
};

// Channel id is the bit position in sensors_present_mask. Stable across
// firmware versions (see spec §3.5). Max value 31 (uint32 mask).
struct SensorChannelId {
    uint8_t value;
};

constexpr uint8_t kSensorChannelIdAir     = 0;
constexpr uint8_t kSensorChannelIdSoil1   = 1;
constexpr uint8_t kSensorChannelIdBattery = 2;
// reserved for Phase 2-4: 3 = Soil2, 4 = Light, 5 = Co2

}
