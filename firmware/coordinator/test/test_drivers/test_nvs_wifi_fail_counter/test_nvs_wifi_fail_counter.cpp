#include <Arduino.h>
#include <unity.h>
#include <Preferences.h>
#include "persistence/NvsWifiFailCounterStore.hpp"

using gh::infra::NvsWifiFailCounterStore;
using gh::domain::ErrorCode;

namespace {
void clearNamespace() {
    Preferences p;
    p.begin("wifi_fail", false);
    p.remove("count");
    p.end();
}
}

void setUp() { clearNamespace(); }
void tearDown() { clearNamespace(); }

void test_load_zero_when_unset() {
    NvsWifiFailCounterStore s;
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(s.reset()));  // ensure clean slate
    TEST_ASSERT_EQUAL_UINT8(0, s.load());
}

void test_increment_then_load() {
    NvsWifiFailCounterStore s;
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(s.reset()));
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(s.increment()));
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(s.increment()));
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(s.increment()));
    TEST_ASSERT_EQUAL_UINT8(3, s.load());
}

void test_reset_zeroes_value() {
    NvsWifiFailCounterStore s;
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(s.increment()));
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(s.reset()));
    TEST_ASSERT_EQUAL_UINT8(0, s.load());
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_load_zero_when_unset);
    RUN_TEST(test_increment_then_load);
    RUN_TEST(test_reset_zeroes_value);
    UNITY_END();
}
void loop() {}
