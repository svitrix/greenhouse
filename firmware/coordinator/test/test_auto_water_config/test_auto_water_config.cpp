#include <unity.h>
#include "entities/AutoWaterConfig.hpp"

using gh::domain::AutoWaterConfig;
using gh::domain::kDefaultAutoWaterConfig;

void test_default_is_valid() {
    TEST_ASSERT_TRUE(kDefaultAutoWaterConfig.valid());
}

void test_trigger_below_lower_bound_invalid() {
    AutoWaterConfig c{.enabled = true, .trigger_below_pct = 4,
                      .min_interval_min = 60, .duration_s = 10};
    TEST_ASSERT_FALSE(c.valid());
}

void test_trigger_above_upper_bound_invalid() {
    AutoWaterConfig c{.enabled = true, .trigger_below_pct = 81,
                      .min_interval_min = 60, .duration_s = 10};
    TEST_ASSERT_FALSE(c.valid());
}

void test_duration_above_20s_invalid() {
    AutoWaterConfig c{.enabled = true, .trigger_below_pct = 30,
                      .min_interval_min = 60, .duration_s = 21};
    TEST_ASSERT_FALSE(c.valid());
}

void test_interval_zero_invalid() {
    AutoWaterConfig c{.enabled = true, .trigger_below_pct = 30,
                      .min_interval_min = 0, .duration_s = 10};
    TEST_ASSERT_FALSE(c.valid());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_default_is_valid);
    RUN_TEST(test_trigger_below_lower_bound_invalid);
    RUN_TEST(test_trigger_above_upper_bound_invalid);
    RUN_TEST(test_duration_above_20s_invalid);
    RUN_TEST(test_interval_zero_invalid);
    return UNITY_END();
}
