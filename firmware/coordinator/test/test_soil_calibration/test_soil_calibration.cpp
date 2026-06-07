#include <unity.h>
#include "entities/SoilCalibration.hpp"

using gh::domain::SoilCalibration;

void test_valid_when_dry_strictly_less_than_wet() {
    constexpr SoilCalibration cal{300, 700};
    TEST_ASSERT_TRUE(cal.valid());
}

void test_invalid_when_dry_equals_wet() {
    constexpr SoilCalibration cal{500, 500};
    TEST_ASSERT_FALSE(cal.valid());
}

void test_invalid_when_dry_greater_than_wet() {
    constexpr SoilCalibration cal{800, 300};
    TEST_ASSERT_FALSE(cal.valid());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_valid_when_dry_strictly_less_than_wet);
    RUN_TEST(test_invalid_when_dry_equals_wet);
    RUN_TEST(test_invalid_when_dry_greater_than_wet);
    return UNITY_END();
}
