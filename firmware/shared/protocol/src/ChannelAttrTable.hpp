#pragma once
#include <cstdint>
#include <cstddef>
#include "Quantity.hpp"
#include "ZclIds.hpp"
#include "entities/SensorKind.hpp"

namespace gh::protocol {

enum class ZclScalar : uint8_t { Int16, Uint16, Uint8 };

struct ChannelAttrEntry {
    uint8_t                channel_id;     // bit position in sensors_present_mask
    gh::domain::SensorKind kind;
    Quantity               quantity;
    uint8_t                endpoint;
    uint16_t               cluster_id;
    uint16_t               attribute_id;
    ZclScalar              scalar;
};

inline constexpr ChannelAttrEntry kChannelAttrTable[] = {
    { gh::domain::kSensorChannelIdAir,     gh::domain::SensorKind::Air,
      Quantity::AirTempC,
      kSensorEndpoint, kClusterTemperatureMeasurement, kAttrMeasuredValue,
      ZclScalar::Int16 },
    { gh::domain::kSensorChannelIdAir,     gh::domain::SensorKind::Air,
      Quantity::AirHumidityPct,
      kSensorEndpoint, kClusterRelativeHumidity, kAttrMeasuredValue,
      ZclScalar::Uint16 },
    { gh::domain::kSensorChannelIdSoil1,   gh::domain::SensorKind::Soil,
      Quantity::SoilMoisturePct,
      kSensorEndpoint, kClusterSoilMoisture, kAttrMeasuredValue,
      ZclScalar::Uint16 },
    { gh::domain::kSensorChannelIdSoil1,   gh::domain::SensorKind::Soil,
      Quantity::SoilTempC,
      kSensorSoilTempEndpoint, kClusterTemperatureMeasurement, kAttrMeasuredValue,
      ZclScalar::Int16 },
    { gh::domain::kSensorChannelIdBattery, gh::domain::SensorKind::Battery,
      Quantity::BatteryPct,
      kSensorEndpoint, kClusterPowerConfiguration, kAttrBatteryPercentageRemaining,
      ZclScalar::Uint8 },
    { gh::domain::kSensorChannelIdBattery, gh::domain::SensorKind::Battery,
      Quantity::BatteryVoltageV,
      kSensorEndpoint, kClusterPowerConfiguration, kAttrBatteryVoltage,
      ZclScalar::Uint8 },
};

inline constexpr size_t kChannelAttrTableSize =
    sizeof(kChannelAttrTable) / sizeof(kChannelAttrTable[0]);

namespace detail {
constexpr bool kAllRowsOfSameKindShareChannelId = [] {
    for (size_t i = 0; i < kChannelAttrTableSize; ++i) {
        for (size_t j = i + 1; j < kChannelAttrTableSize; ++j) {
            if (kChannelAttrTable[i].kind == kChannelAttrTable[j].kind &&
                kChannelAttrTable[i].channel_id != kChannelAttrTable[j].channel_id) {
                return false;
            }
        }
    }
    return true;
}();
static_assert(kAllRowsOfSameKindShareChannelId,
              "kChannelAttrTable: rows with the same SensorKind must share channel_id "
              "(presentMaskFor depends on this).");
}  // namespace detail

[[nodiscard]] constexpr const ChannelAttrEntry*
    findByZclAddress(uint8_t ep, uint16_t cluster, uint16_t attr) noexcept {
    for (size_t i = 0; i < kChannelAttrTableSize; ++i) {
        const auto& e = kChannelAttrTable[i];
        if (e.endpoint == ep && e.cluster_id == cluster && e.attribute_id == attr) {
            return &e;
        }
    }
    return nullptr;
}

// Returns the bit for the first row matching `kind`. Caller invariant: every
// row sharing a `kind` also shares its `channel_id` -- see static_assert above.
[[nodiscard]] constexpr uint32_t presentMaskFor(gh::domain::SensorKind kind) noexcept {
    for (size_t i = 0; i < kChannelAttrTableSize; ++i) {
        if (kChannelAttrTable[i].kind == kind) {
            return 1u << kChannelAttrTable[i].channel_id;
        }
    }
    return 0u;
}

}
