#include <unity.h>
#include "JsonHelpers.hpp"

using gh::presentation::parseIeeeFromPath;
using gh::presentation::kindCode;
using gh::presentation::kindFromCode;
using gh::presentation::quantityFromCode;
using gh::domain::SensorKind;
using gh::protocol::Quantity;

void test_parse_ieee_from_path_extracts_lowercase(void) {
    auto id = parseIeeeFromPath("/api/nodes/00124B001A2B3C4D");
    TEST_ASSERT_TRUE(id.has_value());
    TEST_ASSERT_EQUAL_UINT64(0x00124B001A2B3C4Dull, id->ieee);

    id = parseIeeeFromPath("/api/nodes/00124b001a2b3c4d/alias");
    TEST_ASSERT_TRUE(id.has_value());
    TEST_ASSERT_EQUAL_UINT64(0x00124B001A2B3C4Dull, id->ieee);
}

void test_parse_ieee_rejects_short_or_missing(void) {
    TEST_ASSERT_FALSE(parseIeeeFromPath("/api/nodes/00124B").has_value());
    TEST_ASSERT_FALSE(parseIeeeFromPath("/api/nodes/").has_value());
    TEST_ASSERT_FALSE(parseIeeeFromPath("/").has_value());
}

void test_kindCode_strings(void) {
    TEST_ASSERT_EQUAL_STRING("air",     kindCode(SensorKind::Air));
    TEST_ASSERT_EQUAL_STRING("soil1",   kindCode(SensorKind::Soil));
    TEST_ASSERT_EQUAL_STRING("battery", kindCode(SensorKind::Battery));
}

void test_kindFromCode_roundtrips(void) {
    TEST_ASSERT_EQUAL(SensorKind::Air,     *kindFromCode("air"));
    TEST_ASSERT_EQUAL(SensorKind::Soil,    *kindFromCode("soil1"));
    TEST_ASSERT_EQUAL(SensorKind::Battery, *kindFromCode("battery"));
    TEST_ASSERT_FALSE(kindFromCode("nope").has_value());
}

void test_quantityFromCode_roundtrips(void) {
    TEST_ASSERT_EQUAL(Quantity::AirTempC,        *quantityFromCode("temp_c"));
    TEST_ASSERT_EQUAL(Quantity::AirHumidityPct,  *quantityFromCode("humidity_pct"));
    TEST_ASSERT_EQUAL(Quantity::SoilMoisturePct, *quantityFromCode("moisture_pct"));
    TEST_ASSERT_EQUAL(Quantity::SoilTempC,       *quantityFromCode("soil_temp_c"));
    TEST_ASSERT_EQUAL(Quantity::BatteryPct,      *quantityFromCode("pct"));
    TEST_ASSERT_EQUAL(Quantity::BatteryVoltageV, *quantityFromCode("voltage_v"));
    TEST_ASSERT_FALSE(quantityFromCode("nope").has_value());
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_ieee_from_path_extracts_lowercase);
    RUN_TEST(test_parse_ieee_rejects_short_or_missing);
    RUN_TEST(test_kindCode_strings);
    RUN_TEST(test_kindFromCode_roundtrips);
    RUN_TEST(test_quantityFromCode_roundtrips);
    return UNITY_END();
}
