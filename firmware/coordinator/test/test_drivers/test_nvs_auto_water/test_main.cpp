#include <Arduino.h>
#include <unity.h>
#include <Preferences.h>
#include "persistence/NvsAutoWaterConfigStore.hpp"

using gh::infra::NvsAutoWaterConfigStore;
using gh::domain::AutoWaterConfig;
using gh::domain::ErrorCode;
using gh::domain::kDefaultAutoWaterConfig;

namespace {
void clearNamespace() {
    Preferences p;
    p.begin("auto_water", false);
    p.clear();
    p.end();
}
}

void setUp() { clearNamespace(); }
void tearDown() { clearNamespace(); }

void test_load_returns_defaults_on_first_boot() {
    NvsAutoWaterConfigStore s;
    auto r = s.load();
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok), static_cast<int>(r.err));
    TEST_ASSERT_EQUAL_UINT8(kDefaultAutoWaterConfig.trigger_below_pct,
                            r.value.trigger_below_pct);
    TEST_ASSERT_EQUAL_UINT16(kDefaultAutoWaterConfig.min_interval_min,
                             r.value.min_interval_min);
    TEST_ASSERT_EQUAL_UINT8(kDefaultAutoWaterConfig.duration_s,
                            r.value.duration_s);
    TEST_ASSERT_EQUAL(static_cast<int>(kDefaultAutoWaterConfig.enabled),
                      static_cast<int>(r.value.enabled));
}

void test_save_then_load_round_trip() {
    NvsAutoWaterConfigStore s;
    AutoWaterConfig in{
        .enabled            = true,
        .trigger_below_pct  = 25,
        .min_interval_min   = 120,
        .duration_s         = 12,
    };
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(s.save(in)));
    auto r = s.load();
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok), static_cast<int>(r.err));
    TEST_ASSERT_TRUE(r.value.enabled);
    TEST_ASSERT_EQUAL_UINT8(25,  r.value.trigger_below_pct);
    TEST_ASSERT_EQUAL_UINT16(120, r.value.min_interval_min);
    TEST_ASSERT_EQUAL_UINT8(12,  r.value.duration_s);
}

void test_save_invalid_rejected() {
    NvsAutoWaterConfigStore s;
    // trigger_below_pct=200 is out of range (max 80)
    AutoWaterConfig bad{
        .enabled            = true,
        .trigger_below_pct  = 200,
        .min_interval_min   = 60,
        .duration_s         = 10,
    };
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::ValidationFailed),
                      static_cast<int>(s.save(bad)));
}

void test_corrupt_value_falls_back_to_defaults() {
    NvsAutoWaterConfigStore s;
    AutoWaterConfig in{
        .enabled            = true,
        .trigger_below_pct  = 30,
        .min_interval_min   = 60,
        .duration_s         = 10,
    };
    (void)s.save(in);
    // Manually overwrite trigger with out-of-range garbage
    Preferences p;
    p.begin("auto_water", false);
    p.putUChar("trig", 200);
    p.end();
    auto r = s.load();
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok), static_cast<int>(r.err));
    TEST_ASSERT_EQUAL_UINT8(kDefaultAutoWaterConfig.trigger_below_pct,
                            r.value.trigger_below_pct);
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_load_returns_defaults_on_first_boot);
    RUN_TEST(test_save_then_load_round_trip);
    RUN_TEST(test_save_invalid_rejected);
    RUN_TEST(test_corrupt_value_falls_back_to_defaults);
    UNITY_END();
}
void loop() {}
