#include <unity.h>
#include "telemetry/ChannelToTelemetryMapper.hpp"

using gh::app::ChannelToTelemetryMapper;
using gh::domain::ChannelSample;
using gh::domain::NodeId;
using gh::domain::SensorKind;
using gh::domain::TelemetryKind;
using gh::domain::TelemetryRecord;
using gh::protocol::Quantity;

static ChannelSample sample(SensorKind k, Quantity q, float v) {
    return ChannelSample{k, q, v, /*monotonic_ms*/ 0};
}

void test_air_temp_maps_to_AirTemp(void) {
    const auto r = ChannelToTelemetryMapper::map(
        NodeId{0xA}, sample(SensorKind::Air, Quantity::AirTempC, 23.4f),
        /*unix_ts*/ 100'000);
    TEST_ASSERT_TRUE(r.has_value());
    TEST_ASSERT_EQUAL(TelemetryKind::AirTemp, r->kind);
    TEST_ASSERT_EQUAL_UINT64(100'000ULL, r->ts_unix_ms);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 23.4f, r->value);
}

void test_air_humidity_maps_to_AirHumidity(void) {
    const auto r = ChannelToTelemetryMapper::map(
        NodeId{0xA}, sample(SensorKind::Air, Quantity::AirHumidityPct, 56.2f),
        100'000);
    TEST_ASSERT_TRUE(r.has_value());
    TEST_ASSERT_EQUAL(TelemetryKind::AirHumidity, r->kind);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 56.2f, r->value);
}

void test_soil_moisture_maps_to_SoilMoist(void) {
    const auto r = ChannelToTelemetryMapper::map(
        NodeId{0xA}, sample(SensorKind::Soil, Quantity::SoilMoisturePct, 42.0f),
        100'000);
    TEST_ASSERT_TRUE(r.has_value());
    TEST_ASSERT_EQUAL(TelemetryKind::SoilMoist, r->kind);
    TEST_ASSERT_EQUAL_INT32(gh::domain::kTelemetryRawNotApplicable, r->raw);
}

void test_soil_temp_maps_to_SoilTemp(void) {
    const auto r = ChannelToTelemetryMapper::map(
        NodeId{0xA}, sample(SensorKind::Soil, Quantity::SoilTempC, 18.5f),
        100'000);
    TEST_ASSERT_TRUE(r.has_value());
    TEST_ASSERT_EQUAL(TelemetryKind::SoilTemp, r->kind);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 18.5f, r->value);
}

void test_battery_pct_maps_to_BatteryPct(void) {
    const auto r = ChannelToTelemetryMapper::map(
        NodeId{0xA}, sample(SensorKind::Battery, Quantity::BatteryPct, 87.0f),
        100'000);
    TEST_ASSERT_TRUE(r.has_value());
    TEST_ASSERT_EQUAL(TelemetryKind::BatteryPct, r->kind);
}

void test_battery_voltage_maps_to_BatteryV(void) {
    const auto r = ChannelToTelemetryMapper::map(
        NodeId{0xA}, sample(SensorKind::Battery, Quantity::BatteryVoltageV, 4.1f),
        100'000);
    TEST_ASSERT_TRUE(r.has_value());
    TEST_ASSERT_EQUAL(TelemetryKind::BatteryV, r->kind);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.1f, r->value);
}

void test_unknown_pair_returns_nullopt(void) {
    // Quantity::AirTempC with kind=Battery is a malformed pair.
    const auto r = ChannelToTelemetryMapper::map(
        NodeId{0xA}, sample(SensorKind::Battery, Quantity::AirTempC, 0.0f),
        100'000);
    TEST_ASSERT_FALSE(r.has_value());
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_air_temp_maps_to_AirTemp);
    RUN_TEST(test_air_humidity_maps_to_AirHumidity);
    RUN_TEST(test_soil_moisture_maps_to_SoilMoist);
    RUN_TEST(test_soil_temp_maps_to_SoilTemp);
    RUN_TEST(test_battery_pct_maps_to_BatteryPct);
    RUN_TEST(test_battery_voltage_maps_to_BatteryV);
    RUN_TEST(test_unknown_pair_returns_nullopt);
    return UNITY_END();
}
