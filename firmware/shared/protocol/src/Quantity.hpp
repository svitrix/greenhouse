#pragma once
#include <cstdint>

namespace gh::protocol {

enum class Quantity : uint8_t {
    AirTempC         = 0,
    AirHumidityPct   = 1,
    SoilMoisturePct  = 2,
    SoilTempC        = 3,
    BatteryPct       = 4,
    BatteryVoltageV  = 5,
};

[[nodiscard]] const char* quantityCode(Quantity q) noexcept;

}
