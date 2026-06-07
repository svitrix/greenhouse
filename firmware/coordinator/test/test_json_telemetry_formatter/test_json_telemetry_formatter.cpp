#include <unity.h>
#include <cstring>
#include "JsonTelemetryFormatter.hpp"
#include "entities/SoilSample.hpp"
#include "entities/AirSample.hpp"
#include "entities/PumpState.hpp"

using gh::domain::SoilSample;
using gh::domain::AirSample;
using gh::domain::PumpState;
namespace fmt = gh::app::JsonTelemetryFormatter;

void test_format_soil_exact_match() {
    SoilSample s{12345, 520, 36, 215};
    char buf[128];
    int n = fmt::formatSoil(s, buf, sizeof(buf));
    const char* expected =
        "{\"t\":12345,\"soil\":{\"raw\":520,\"pct\":36,\"temp_c10\":215}}\n";
    TEST_ASSERT_EQUAL_INT((int)strlen(expected), n);
    TEST_ASSERT_EQUAL_STRING(expected, buf);
}

void test_format_air_exact_match() {
    AirSample s{12345, 240, 555};
    char buf[128];
    int n = fmt::formatAir(s, buf, sizeof(buf));
    const char* expected =
        "{\"t\":12345,\"air\":{\"temp_c10\":240,\"rh_x10\":555}}\n";
    TEST_ASSERT_EQUAL_INT((int)strlen(expected), n);
    TEST_ASSERT_EQUAL_STRING(expected, buf);
}

void test_format_pump_off() {
    char buf[64];
    int n = fmt::formatPump(PumpState::Off, 12345, buf, sizeof(buf));
    const char* expected = "{\"t\":12345,\"pump\":\"OFF\"}\n";
    TEST_ASSERT_EQUAL_INT((int)strlen(expected), n);
    TEST_ASSERT_EQUAL_STRING(expected, buf);
}

void test_format_pump_on() {
    char buf[64];
    (void)fmt::formatPump(PumpState::On, 12345, buf, sizeof(buf));
    const char* expected = "{\"t\":12345,\"pump\":\"ON\"}\n";
    TEST_ASSERT_EQUAL_STRING(expected, buf);
}

void test_format_pump_safety_locked() {
    char buf[64];
    (void)fmt::formatPump(PumpState::SafetyLocked, 12345, buf, sizeof(buf));
    const char* expected = "{\"t\":12345,\"pump\":\"LOCKED\"}\n";
    TEST_ASSERT_EQUAL_STRING(expected, buf);
}

void test_format_negative_temperature() {
    SoilSample s{99, 280, 0, -75};
    char buf[128];
    int n = fmt::formatSoil(s, buf, sizeof(buf));
    const char* expected =
        "{\"t\":99,\"soil\":{\"raw\":280,\"pct\":0,\"temp_c10\":-75}}\n";
    TEST_ASSERT_EQUAL_INT((int)strlen(expected), n);
    TEST_ASSERT_EQUAL_STRING(expected, buf);
}

void test_format_overflow_returns_minus_one() {
    SoilSample s{12345, 520, 36, 215};
    char buf[8];
    int n = fmt::formatSoil(s, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(-1, n);
}

void test_format_air_max_humidity() {
    AirSample s{0, 850, 1000};
    char buf[128];
    (void)fmt::formatAir(s, buf, sizeof(buf));
    const char* expected =
        "{\"t\":0,\"air\":{\"temp_c10\":850,\"rh_x10\":1000}}\n";
    TEST_ASSERT_EQUAL_STRING(expected, buf);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_format_soil_exact_match);
    RUN_TEST(test_format_air_exact_match);
    RUN_TEST(test_format_pump_off);
    RUN_TEST(test_format_pump_on);
    RUN_TEST(test_format_pump_safety_locked);
    RUN_TEST(test_format_negative_temperature);
    RUN_TEST(test_format_overflow_returns_minus_one);
    RUN_TEST(test_format_air_max_humidity);
    return UNITY_END();
}
