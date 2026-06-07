#pragma once
#include <cstdint>
#include "entities/SensorKind.hpp"

namespace gh::domain {

enum class TelemetryKind : uint8_t {
    AirTemp     = 0,
    AirHumidity = 1,
    SoilMoist   = 2,
    SoilTemp    = 3,
    BatteryPct  = 4,
    BatteryV    = 5,
};

constexpr const char* telemetryKindWire(TelemetryKind k) noexcept {
    switch (k) {
        case TelemetryKind::AirTemp:     return "air_temp";
        case TelemetryKind::AirHumidity: return "air_humidity";
        case TelemetryKind::SoilMoist:   return "soil_moist";
        case TelemetryKind::SoilTemp:    return "soil_temp";
        case TelemetryKind::BatteryPct:  return "battery_pct";
        case TelemetryKind::BatteryV:    return "battery_v";
    }
    return "unknown";
}

constexpr int32_t kTelemetryRawNotApplicable = -1;

struct TelemetryRecord {
    uint64_t      ts_unix_ms;
    uint8_t       channel_id;
    TelemetryKind kind;
    float         value;
    int32_t       raw;
    uint8_t       status;
};

}
