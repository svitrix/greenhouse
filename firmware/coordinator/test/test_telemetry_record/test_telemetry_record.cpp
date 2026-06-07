#include <unity.h>
#include <type_traits>
#include <cstring>
#include "entities/TelemetryRecord.hpp"

using namespace gh::domain;

void test_wire_strings_are_stable() {
    TEST_ASSERT_EQUAL_STRING("air_temp",     telemetryKindWire(TelemetryKind::AirTemp));
    TEST_ASSERT_EQUAL_STRING("air_humidity", telemetryKindWire(TelemetryKind::AirHumidity));
    TEST_ASSERT_EQUAL_STRING("soil_moist",   telemetryKindWire(TelemetryKind::SoilMoist));
    TEST_ASSERT_EQUAL_STRING("soil_temp",    telemetryKindWire(TelemetryKind::SoilTemp));
    TEST_ASSERT_EQUAL_STRING("battery_pct",  telemetryKindWire(TelemetryKind::BatteryPct));
    TEST_ASSERT_EQUAL_STRING("battery_v",    telemetryKindWire(TelemetryKind::BatteryV));
}

void test_record_is_trivially_copyable() {
    // The queue layer stores records as raw bytes in a LittleFS ring;
    // trivial copyability is a load-bearing assumption.
    TEST_ASSERT_TRUE(std::is_trivially_copyable<TelemetryRecord>::value);
}

void test_raw_sentinel() {
    TelemetryRecord r{0, 0, TelemetryKind::AirTemp, 0.0f, kTelemetryRawNotApplicable, 0};
    TEST_ASSERT_EQUAL_INT32(-1, r.raw);
    TEST_ASSERT_EQUAL_INT32(-1, kTelemetryRawNotApplicable);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_wire_strings_are_stable);
    RUN_TEST(test_record_is_trivially_copyable);
    RUN_TEST(test_raw_sentinel);
    return UNITY_END();
}
