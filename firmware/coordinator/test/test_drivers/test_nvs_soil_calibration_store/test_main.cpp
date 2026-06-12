#include <Arduino.h>
#include <unity.h>
#include <Preferences.h>
#include "persistence/NvsSoilCalibrationStore.hpp"

using gh::domain::ErrorCode;
using gh::domain::SoilCalibration;
using gh::infra::NvsSoilCalibrationStore;

namespace {
void clearNamespace() {
    Preferences p;
    p.begin("soil_cal", false);
    p.clear();
    p.end();
}
}

void setUp() { clearNamespace(); }
void tearDown() { clearNamespace(); }

void test_load_on_empty_returns_ConfigNotFound() {
    NvsSoilCalibrationStore store;
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(store.begin()));
    auto loaded = store.load();
    TEST_ASSERT_FALSE(loaded.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::ConfigNotFound),
                      static_cast<int>(loaded.err));
}

void test_save_then_load_returns_same_value() {
    NvsSoilCalibrationStore store;
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(store.begin()));
    SoilCalibration cal{350, 750};
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(store.save(cal)));
    auto loaded = store.load();
    TEST_ASSERT_TRUE(loaded.ok());
    TEST_ASSERT_EQUAL_UINT16(350, loaded.value.raw_dry);
    TEST_ASSERT_EQUAL_UINT16(750, loaded.value.raw_wet);
}

void test_save_invalid_returns_SensorOutOfRange() {
    NvsSoilCalibrationStore store;
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(store.begin()));
    SoilCalibration bad{900, 300};  // dry > wet
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::SensorOutOfRange),
                      static_cast<int>(store.save(bad)));
}

void test_save_overwrites_previous() {
    NvsSoilCalibrationStore store;
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(store.begin()));
    store.save(SoilCalibration{.raw_dry = 300, .raw_wet = 700});
    store.save(SoilCalibration{.raw_dry = 320, .raw_wet = 720});
    auto loaded = store.load();
    TEST_ASSERT_TRUE(loaded.ok());
    TEST_ASSERT_EQUAL_UINT16(320, loaded.value.raw_dry);
    TEST_ASSERT_EQUAL_UINT16(720, loaded.value.raw_wet);
}

void test_load_without_begin_returns_SensorNotReady() {
    NvsSoilCalibrationStore store;
    auto loaded = store.load();
    TEST_ASSERT_FALSE(loaded.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::SensorNotReady),
                      static_cast<int>(loaded.err));
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_load_on_empty_returns_ConfigNotFound);
    RUN_TEST(test_save_then_load_returns_same_value);
    RUN_TEST(test_save_invalid_returns_SensorOutOfRange);
    RUN_TEST(test_save_overwrites_previous);
    RUN_TEST(test_load_without_begin_returns_SensorNotReady);
    UNITY_END();
}

void loop() {}
