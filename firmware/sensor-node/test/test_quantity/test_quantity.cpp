#include <unity.h>
#include <cstring>
#include "Quantity.hpp"

using gh::protocol::Quantity;
using gh::protocol::quantityCode;

void test_quantityCode_returns_stable_wire_strings(void) {
    TEST_ASSERT_EQUAL_STRING("temp_c",        quantityCode(Quantity::AirTempC));
    TEST_ASSERT_EQUAL_STRING("humidity_pct",  quantityCode(Quantity::AirHumidityPct));
    TEST_ASSERT_EQUAL_STRING("moisture_pct",  quantityCode(Quantity::SoilMoisturePct));
    TEST_ASSERT_EQUAL_STRING("soil_temp_c",   quantityCode(Quantity::SoilTempC));
    TEST_ASSERT_EQUAL_STRING("pct",           quantityCode(Quantity::BatteryPct));
    TEST_ASSERT_EQUAL_STRING("voltage_v",     quantityCode(Quantity::BatteryVoltageV));
}

void test_quantityCode_unknown_returns_empty(void) {
    const auto code = quantityCode(static_cast<Quantity>(0xEE));
    TEST_ASSERT_EQUAL_STRING("", code);
}

void setUp() {}
void tearDown() {}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_quantityCode_returns_stable_wire_strings);
    RUN_TEST(test_quantityCode_unknown_returns_empty);
    return UNITY_END();
}
