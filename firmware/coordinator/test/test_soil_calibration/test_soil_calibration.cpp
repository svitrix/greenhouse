#include <unity.h>
#include "entities/SoilCalibration.hpp"

using gh::domain::SoilCalibration;

void test_valid_when_dry_strictly_less_than_wet() {
    constexpr SoilCalibration cal{.raw_dry = 300, .raw_wet = 700};
    TEST_ASSERT_TRUE(cal.valid());
}

void test_invalid_when_dry_equals_wet() {
    constexpr SoilCalibration cal{.raw_dry = 500, .raw_wet = 500};
    TEST_ASSERT_FALSE(cal.valid());
}

void test_invalid_when_dry_greater_than_wet() {
    constexpr SoilCalibration cal{.raw_dry = 800, .raw_wet = 300};
    TEST_ASSERT_FALSE(cal.valid());
}

void test_default_schema_version_is_current() {
    constexpr SoilCalibration cal{.raw_dry = 300, .raw_wet = 700};
    TEST_ASSERT_EQUAL_UINT8(gh::domain::kSoilCalibrationSchemaVersion,
                            cal.schema_version);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_valid_when_dry_strictly_less_than_wet);
    RUN_TEST(test_invalid_when_dry_equals_wet);
    RUN_TEST(test_invalid_when_dry_greater_than_wet);
    RUN_TEST(test_default_schema_version_is_current);
    return UNITY_END();
}
