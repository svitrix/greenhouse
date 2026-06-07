#include <Arduino.h>
#include <unity.h>
#include "persistence/NvsLastConnectErrorStore.hpp"

using gh::infra::NvsLastConnectErrorStore;
using gh::domain::ErrorCode;
using gh::domain::ConnectError;

void test_load_none_when_unset() {
    NvsLastConnectErrorStore s;
    TEST_ASSERT_EQUAL(ErrorCode::Ok, s.save(ConnectError::None));
    TEST_ASSERT_EQUAL(static_cast<int>(ConnectError::None),
                      static_cast<int>(s.load()));
}

void test_save_and_load_each_value() {
    NvsLastConnectErrorStore s;
    for (auto v : { ConnectError::AuthFail, ConnectError::SsidNotFound,
                    ConnectError::Timeout,  ConnectError::Other }) {
        TEST_ASSERT_EQUAL(ErrorCode::Ok, s.save(v));
        TEST_ASSERT_EQUAL(static_cast<int>(v), static_cast<int>(s.load()));
    }
    TEST_ASSERT_EQUAL(ErrorCode::Ok, s.save(ConnectError::None));
}

void setUp(void) {}
void tearDown(void) {}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_load_none_when_unset);
    RUN_TEST(test_save_and_load_each_value);
    UNITY_END();
}

void loop() {}
