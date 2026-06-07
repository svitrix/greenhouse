#include "JsonHelpers.hpp"

namespace gh::presentation {

std::optional<gh::domain::NodeId>
parseIeeeFromPath(std::string_view path) noexcept {
    size_t pos = 0;
    while (pos < path.size()) {
        const size_t slash = path.find('/', pos);
        const size_t end   = (slash == std::string_view::npos) ? path.size() : slash;
        if (end - pos == 16) {
            if (auto id = gh::domain::NodeId::parseHex16(path.substr(pos, 16))) {
                return id;
            }
        }
        if (slash == std::string_view::npos) break;
        pos = slash + 1;
    }
    return std::nullopt;
}

const char* kindCode(gh::domain::SensorKind k) noexcept {
    switch (k) {
        case gh::domain::SensorKind::Air:     return "air";
        case gh::domain::SensorKind::Soil:    return "soil1";
        case gh::domain::SensorKind::Battery: return "battery";
        case gh::domain::SensorKind::Light:   return "";
        case gh::domain::SensorKind::Co2:     return "";
    }
    return "";
}

std::optional<gh::domain::SensorKind> kindFromCode(std::string_view s) noexcept {
    if (s == "air")     return gh::domain::SensorKind::Air;
    if (s == "soil1")   return gh::domain::SensorKind::Soil;
    if (s == "battery") return gh::domain::SensorKind::Battery;
    return std::nullopt;
}

std::optional<gh::protocol::Quantity>
quantityFromCode(std::string_view s) noexcept {
    if (s == "temp_c")       return gh::protocol::Quantity::AirTempC;
    if (s == "humidity_pct") return gh::protocol::Quantity::AirHumidityPct;
    if (s == "moisture_pct") return gh::protocol::Quantity::SoilMoisturePct;
    if (s == "soil_temp_c")  return gh::protocol::Quantity::SoilTempC;
    if (s == "pct")          return gh::protocol::Quantity::BatteryPct;
    if (s == "voltage_v")    return gh::protocol::Quantity::BatteryVoltageV;
    return std::nullopt;
}

}
