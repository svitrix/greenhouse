#pragma once
#include <cstdint>
#include <cstddef>
#include "entities/SensorReading.hpp"
#include "ports/IZigbeeEndDevice.hpp"
#include "ChannelAttrTable.hpp"     // <-- shared
#include "ZclIds.hpp"

namespace gh::infra {

using AttrEncoder = void (*)(const gh::domain::SensorReading&,
                             uint8_t* out_buf, size_t* out_size) noexcept;

struct AttrMapping {
    uint8_t                endpoint;
    uint16_t               cluster_id;
    uint16_t               attribute_id;
    gh::domain::ZclType    type;
    AttrEncoder            encode;
};

struct ChannelMapping {
    uint8_t                channel_id_value;
    gh::domain::SensorKind expected_kind;
    const AttrMapping*     attrs;
    size_t                 attr_count;
};

// Encoders unchanged — local to sensor-node (encoding direction).
inline void encodeAirTemp(const gh::domain::SensorReading& r,
                          uint8_t* out, size_t* sz) noexcept {
    const int16_t v = gh::protocol::airTempToZcl(r.values.air.temperature_c_x10);
    out[0] = static_cast<uint8_t>( static_cast<uint16_t>(v)        & 0xFFU);
    out[1] = static_cast<uint8_t>((static_cast<uint16_t>(v) >> 8U) & 0xFFU);
    *sz = 2;
}
inline void encodeAirHum(const gh::domain::SensorReading& r,
                         uint8_t* out, size_t* sz) noexcept {
    const uint16_t v = gh::protocol::airHumidityToZcl(r.values.air.humidity_pct_x10);
    out[0] = static_cast<uint8_t>( v        & 0xFFU);
    out[1] = static_cast<uint8_t>((v >> 8U) & 0xFFU);
    *sz = 2;
}
inline void encodeSoilMoist(const gh::domain::SensorReading& r,
                            uint8_t* out, size_t* sz) noexcept {
    const uint16_t v = gh::protocol::soilRawToZcl(r.values.soil.raw_capacitance);
    out[0] = static_cast<uint8_t>( v        & 0xFFU);
    out[1] = static_cast<uint8_t>((v >> 8U) & 0xFFU);
    *sz = 2;
}
inline void encodeSoilTemp(const gh::domain::SensorReading& r,
                           uint8_t* out, size_t* sz) noexcept {
    const int16_t v = gh::protocol::airTempToZcl(r.values.soil.temperature_c_x10);
    out[0] = static_cast<uint8_t>( static_cast<uint16_t>(v)        & 0xFFU);
    out[1] = static_cast<uint8_t>((static_cast<uint16_t>(v) >> 8U) & 0xFFU);
    *sz = 2;
}
inline void encodeBatteryPct(const gh::domain::SensorReading& r,
                             uint8_t* out, size_t* sz) noexcept {
    out[0] = gh::protocol::batteryPctToZcl(r.values.battery.state_of_charge_pct);
    *sz = 1;
}
inline void encodeBatteryVoltage(const gh::domain::SensorReading& r,
                                 uint8_t* out, size_t* sz) noexcept {
    const uint32_t mv    = r.values.battery.voltage_mv;
    const uint32_t units = (mv + 50U) / 100U;
    out[0] = static_cast<uint8_t>(units > 255U ? 255U : units);
    *sz = 1;
}

namespace detail {
// Compile-time lookup: assert the shared table contains the attribute and
// return its address triple. Hard-fail at compile time on a typo.
constexpr gh::protocol::ChannelAttrEntry sharedEntryFor(
    gh::protocol::Quantity q) noexcept {
    for (size_t i = 0; i < gh::protocol::kChannelAttrTableSize; ++i) {
        if (gh::protocol::kChannelAttrTable[i].quantity == q) {
            return gh::protocol::kChannelAttrTable[i];
        }
    }
    return {0, gh::domain::SensorKind::Air, q, 0, 0, 0,
            gh::protocol::ZclScalar::Uint8};  // unreachable for valid q
}

constexpr AttrMapping mapping(gh::protocol::Quantity q,
                              gh::domain::ZclType type,
                              AttrEncoder enc) noexcept {
    const auto e = sharedEntryFor(q);
    return AttrMapping{e.endpoint, e.cluster_id, e.attribute_id, type, enc};
}
} // namespace detail

inline constexpr AttrMapping kAirAttrs[] = {
    detail::mapping(gh::protocol::Quantity::AirTempC,
                    gh::domain::ZclType::Int16,  &encodeAirTemp),
    detail::mapping(gh::protocol::Quantity::AirHumidityPct,
                    gh::domain::ZclType::Uint16, &encodeAirHum),
};

inline constexpr AttrMapping kSoilAttrs[] = {
    detail::mapping(gh::protocol::Quantity::SoilMoisturePct,
                    gh::domain::ZclType::Uint16, &encodeSoilMoist),
    detail::mapping(gh::protocol::Quantity::SoilTempC,
                    gh::domain::ZclType::Int16,  &encodeSoilTemp),
};

inline constexpr AttrMapping kBatteryAttrs[] = {
    detail::mapping(gh::protocol::Quantity::BatteryPct,
                    gh::domain::ZclType::Uint8,  &encodeBatteryPct),
    detail::mapping(gh::protocol::Quantity::BatteryVoltageV,
                    gh::domain::ZclType::Uint8,  &encodeBatteryVoltage),
};

inline constexpr ChannelMapping kChannelMappings[] = {
    { gh::domain::kSensorChannelIdAir,     gh::domain::SensorKind::Air,
      kAirAttrs,     sizeof(kAirAttrs)     / sizeof(kAirAttrs[0])     },
    { gh::domain::kSensorChannelIdSoil1,   gh::domain::SensorKind::Soil,
      kSoilAttrs,    sizeof(kSoilAttrs)    / sizeof(kSoilAttrs[0])    },
    { gh::domain::kSensorChannelIdBattery, gh::domain::SensorKind::Battery,
      kBatteryAttrs, sizeof(kBatteryAttrs) / sizeof(kBatteryAttrs[0]) },
};
inline constexpr size_t kChannelMappingsCount =
    sizeof(kChannelMappings) / sizeof(kChannelMappings[0]);

}
