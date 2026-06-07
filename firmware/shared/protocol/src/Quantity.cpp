#include "Quantity.hpp"

namespace gh::protocol {

const char* quantityCode(Quantity q) noexcept {
    switch (q) {
        case Quantity::AirTempC:        return "temp_c";
        case Quantity::AirHumidityPct:  return "humidity_pct";
        case Quantity::SoilMoisturePct: return "moisture_pct";
        case Quantity::SoilTempC:       return "soil_temp_c";
        case Quantity::BatteryPct:      return "pct";
        case Quantity::BatteryVoltageV: return "voltage_v";
    }
    return "";
}

}
