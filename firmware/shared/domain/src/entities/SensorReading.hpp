#pragma once
#include <cstdint>
#include "entities/SensorKind.hpp"
#include "entities/AirSample.hpp"
#include "entities/SoilSample.hpp"
#include "entities/BatteryReading.hpp"

namespace gh::domain {

struct LightReading {
    uint32_t illuminance_lux;
};

struct Co2Reading {
    uint16_t co2_ppm;
    int16_t  temperature_c_x10;
    uint16_t humidity_pct_x10;
};

struct SensorReading {
    SensorChannelId id;
    SensorKind      kind;
    uint32_t        read_at_ms;
    union {
        AirSample      air;
        SoilSample     soil;
        BatteryReading battery;
        LightReading   light;
        Co2Reading     co2;
    } values;
};

// All five member types are trivial POD today; if a future addition is
// non-trivial, switch to std::variant or a tagged union with manual
// lifetime management.
static_assert(sizeof(SensorReading) > 0, "SensorReading must be defined");

}
