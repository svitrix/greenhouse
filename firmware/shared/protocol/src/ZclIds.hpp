#pragma once
#include <cstdint>

namespace gh::protocol {

// Endpoint layout (sensor-node side; coordinator just listens):
//   ep 1 = sensor-node primary endpoint, hosts all clusters below.
//   ep 2 = secondary endpoint, hosts one TempMeasurement cluster
//          carrying the Chirp soil temperature (int16, centi-°C).
//          Kept on a separate endpoint so EP1's cluster list stays
//          clean and interoperable with standard HA Zigbee integration.
constexpr uint8_t  kSensorEndpoint              = 1;
constexpr uint8_t  kSensorSoilTempEndpoint      = 2;

// Standard ZCL clusters
constexpr uint16_t kClusterBasic                = 0x0000;
constexpr uint16_t kClusterPowerConfiguration   = 0x0001;
constexpr uint16_t kClusterTemperatureMeasurement = 0x0402;
constexpr uint16_t kClusterRelativeHumidity     = 0x0405;
// Soil moisture payload cluster. ZCL defines 0x0408 as Soil Moisture but
// esp-zigbee-lib does not implement it; add_custom_cluster() requires
// manufacturer cluster IDs (> 0x8000). Both firmwares use 0xFC08.
constexpr uint16_t kClusterSoilMoisture         = 0xFC08;

// Standard ZCL attributes (within cluster)
constexpr uint16_t kAttrBasicManufacturerName   = 0x0004;
constexpr uint16_t kAttrBasicModelIdentifier    = 0x0005;
// Custom attribute, manufacturer-specific. Holds desired report period (seconds).
// Coordinator writes; sensor-node reads on every wake before computing sleep.
constexpr uint16_t kAttrBasicReportPeriodS      = 0xFF00;
// Manufacturer-specific attribute (range 0xF000-0xFFFE). uint32 bitmask
// where bit n = SensorChannelId{n} reported Ok this cycle. Surfaces
// per-channel health to Home Assistant without per-channel attributes.
constexpr uint16_t kAttrBasicSensorsPresentMask = 0xF001;

constexpr uint16_t kAttrBatteryPercentageRemaining = 0x0021;
constexpr uint16_t kAttrBatteryVoltage          = 0x0020;

// All measurement clusters use attribute 0x0000 "MeasuredValue".
constexpr uint16_t kAttrMeasuredValue           = 0x0000;

// Manufacturer code (we make one up — not registered with ZCL Alliance).
constexpr uint16_t kManufacturerCode            = 0xFFEE;

// Manufacturer / model strings published in Basic cluster.
constexpr char     kBasicManufacturer[]         = "diy-greenhouse";
// Bumped to v2 when endpoint 2 (soil temperature) was added.
// HA caches device descriptors by model id; v1 devices had only 4 clusters
// on EP1. Bumping prevents HA from confusing old and new firmware.
constexpr char     kBasicModelSensorNode[]      = "gh-sensor-node-v2";

// Default report period bounds (sensor-node clamps received writes).
constexpr uint32_t kReportPeriodDefaultS        = 60;
constexpr uint32_t kReportPeriodMinS            = 60;
constexpr uint32_t kReportPeriodMaxS            = 3600;

// ----------- Encoding helpers (ZCL convention) -----------

// Temperature: int16, centi-°C. AirSample.temperature_c_x10 is decis-°C
// — multiply by 10 to convert.
[[nodiscard]] constexpr int16_t airTempToZcl(int16_t temp_c_x10) noexcept {
    return static_cast<int16_t>(temp_c_x10 * 10);
}
[[nodiscard]] constexpr int16_t airTempFromZcl(int16_t zcl_value) noexcept {
    return static_cast<int16_t>(zcl_value / 10);
}

// Humidity: uint16, centi-%. AirSample.humidity_pct_x10 is decis-%.
[[nodiscard]] constexpr uint16_t airHumidityToZcl(uint16_t hum_pct_x10) noexcept {
    return static_cast<uint16_t>(hum_pct_x10 * 10);
}
[[nodiscard]] constexpr uint16_t airHumidityFromZcl(uint16_t zcl_value) noexcept {
    return static_cast<uint16_t>(zcl_value / 10);
}

// Soil moisture: uint16, centi-%. We're sending the RAW Chirp capacitance
// reading scaled into the ZCL 0..10000 range to keep ZHA-ish semantics.
// Coordinator's SoilNormalizer converts to true % using calibration.
// raw 0..1023 → 0..10000 zcl value (multiply by ~9.78).
[[nodiscard]] constexpr uint16_t soilRawToZcl(uint16_t raw) noexcept {
    // raw is 0..1023; saturate at 10000.
    const uint32_t scaled = static_cast<uint32_t>(raw) * 10000U / 1023U;
    return scaled > 10000U ? static_cast<uint16_t>(10000)
                           : static_cast<uint16_t>(scaled);
}
[[nodiscard]] constexpr uint16_t soilRawFromZcl(uint16_t zcl_value) noexcept {
    const uint32_t raw = static_cast<uint32_t>(zcl_value) * 1023U / 10000U;
    return raw > 1023U ? static_cast<uint16_t>(1023) : static_cast<uint16_t>(raw);
}

// Battery: 8-bit, value × 2 (ZCL convention — half-percentage units).
// 0..200 representing 0..100%.
[[nodiscard]] constexpr uint8_t batteryPctToZcl(uint8_t pct) noexcept {
    return pct > 100 ? static_cast<uint8_t>(200)
                     : static_cast<uint8_t>(pct * 2U);
}
[[nodiscard]] constexpr uint8_t batteryPctFromZcl(uint8_t zcl_value) noexcept {
    return static_cast<uint8_t>(zcl_value / 2U);
}

}
