#include <Arduino.h>
#include <unity.h>
#include <cstring>
#include "NvsAnalyticsConfigStore.hpp"

using namespace gh::infra;
using namespace gh::domain;

void test_load_returns_config_not_found_when_empty() {
    NvsAnalyticsConfigStore s;
    s.clear();
    AnalyticsConfig cfg{};
    // Error codes unified with the other Nvs*Stores: ConfigNotFound replaces the
    // former store-specific NotFound.
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::ConfigNotFound),
                      static_cast<int>(s.load(cfg)));
}

void test_save_rejects_overlong_url() {
    NvsAnalyticsConfigStore s;
    s.clear();
    AnalyticsConfig in{};
    std::memset(in.backend_url, 'u', sizeof(in.backend_url));  // no NUL in bounds
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::ValidationFailed),
                      static_cast<int>(s.save(in)));
}

void test_save_and_load_roundtrip() {
    NvsAnalyticsConfigStore s;
    s.clear();
    AnalyticsConfig in{};
    std::strcpy(in.backend_url, "https://example.com/ingest");
    std::strcpy(in.api_key, "0123456789abcdef");
    in.flush_period_s = 600;
    in.insecure_tls   = true;
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(s.save(in)));

    AnalyticsConfig out{};
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(s.load(out)));
    TEST_ASSERT_EQUAL_STRING(in.backend_url, out.backend_url);
    TEST_ASSERT_EQUAL_STRING(in.api_key, out.api_key);
    TEST_ASSERT_EQUAL_UINT(600, out.flush_period_s);
    TEST_ASSERT_TRUE(out.insecure_tls);
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_load_returns_config_not_found_when_empty);
    RUN_TEST(test_save_rejects_overlong_url);
    RUN_TEST(test_save_and_load_roundtrip);
    UNITY_END();
}

void loop() {}
