#include <unity.h>
#include <array>
#include "network/ZclSensorMapper.hpp"
#include "ZclIds.hpp"

using gh::domain::SensorKind;
using gh::infra::ZclSensorMapper;
using gh::protocol::Quantity;

static std::array<uint8_t, 2> le16(int16_t v) {
    return { static_cast<uint8_t>( static_cast<uint16_t>(v)        & 0xFFu),
             static_cast<uint8_t>((static_cast<uint16_t>(v) >> 8u) & 0xFFu) };
}

void test_air_temp_decodes(void) {
    const auto raw = le16(2340);
    auto d = ZclSensorMapper::decode(1, 0x0402, 0x0000, raw.data(), raw.size());
    TEST_ASSERT_TRUE(d.has_value());
    TEST_ASSERT_EQUAL(SensorKind::Air,   d->kind);
    TEST_ASSERT_EQUAL(Quantity::AirTempC, d->quantity);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 23.4f, d->value_si);
}

void test_air_humidity_decodes(void) {
    const auto raw = le16(static_cast<int16_t>(5620));
    auto d = ZclSensorMapper::decode(1, 0x0405, 0x0000, raw.data(), raw.size());
    TEST_ASSERT_TRUE(d.has_value());
    TEST_ASSERT_EQUAL(Quantity::AirHumidityPct, d->quantity);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 56.2f, d->value_si);
}

void test_soil_moisture_passes_through_zcl_units(void) {
    const auto raw = le16(4200);
    auto d = ZclSensorMapper::decode(1, gh::protocol::kClusterSoilMoisture, 0x0000, raw.data(), raw.size());
    TEST_ASSERT_TRUE(d.has_value());
    TEST_ASSERT_EQUAL(Quantity::SoilMoisturePct, d->quantity);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 42.0f, d->value_si);
}

void test_soil_temp_on_ep2(void) {
    const auto raw = le16(1850);
    auto d = ZclSensorMapper::decode(2, 0x0402, 0x0000, raw.data(), raw.size());
    TEST_ASSERT_TRUE(d.has_value());
    TEST_ASSERT_EQUAL(Quantity::SoilTempC, d->quantity);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 18.5f, d->value_si);
}

void test_battery_pct_decodes(void) {
    const uint8_t raw = 174;
    auto d = ZclSensorMapper::decode(1, 0x0001, 0x0021, &raw, 1);
    TEST_ASSERT_TRUE(d.has_value());
    TEST_ASSERT_EQUAL(Quantity::BatteryPct, d->quantity);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 87.0f, d->value_si);
}

void test_battery_voltage_decodes(void) {
    const uint8_t raw = 41;
    auto d = ZclSensorMapper::decode(1, 0x0001, 0x0020, &raw, 1);
    TEST_ASSERT_TRUE(d.has_value());
    TEST_ASSERT_EQUAL(Quantity::BatteryVoltageV, d->quantity);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.1f, d->value_si);
}

void test_unknown_address_returns_nullopt(void) {
    const uint8_t raw[2] = { 0, 0 };
    TEST_ASSERT_FALSE(ZclSensorMapper::decode(7, 0x1234, 0x5678, raw, 2).has_value());
}

void test_truncated_payload_returns_nullopt(void) {
    const uint8_t raw[1] = { 0 };
    TEST_ASSERT_FALSE(ZclSensorMapper::decode(1, 0x0402, 0x0000, raw, 1).has_value());
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_air_temp_decodes);
    RUN_TEST(test_air_humidity_decodes);
    RUN_TEST(test_soil_moisture_passes_through_zcl_units);
    RUN_TEST(test_soil_temp_on_ep2);
    RUN_TEST(test_battery_pct_decodes);
    RUN_TEST(test_battery_voltage_decodes);
    RUN_TEST(test_unknown_address_returns_nullopt);
    RUN_TEST(test_truncated_payload_returns_nullopt);
    return UNITY_END();
}
