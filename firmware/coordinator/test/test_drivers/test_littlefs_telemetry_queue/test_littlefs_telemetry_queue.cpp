#include <Arduino.h>
#include <unity.h>
#include "LittleFsTelemetryQueue.hpp"

using namespace gh::infra;
using namespace gh::domain;

static TelemetryRecord rec(uint64_t ts, float v) {
    return TelemetryRecord{ts, 0, TelemetryKind::AirTemp, v,
                           kTelemetryRawNotApplicable, 0};
}

void test_begin_then_clear_is_empty() {
    LittleFsTelemetryQueue q;
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(q.begin()));
    q.clear();
    TEST_ASSERT_EQUAL_UINT(0, q.size());
}

void test_append_size_peek_drop_roundtrip() {
    LittleFsTelemetryQueue q;
    q.begin();
    q.clear();
    for (uint64_t i = 1; i <= 5; ++i) {
        TEST_ASSERT_EQUAL(
            static_cast<int>(ErrorCode::Ok),
            static_cast<int>(q.append(rec(i, static_cast<float>(i))))
        );
    }
    TEST_ASSERT_EQUAL_UINT(5, q.size());

    TelemetryRecord out[10];
    size_t n = q.peek(out, 10);
    TEST_ASSERT_EQUAL_UINT(5, n);
    TEST_ASSERT_EQUAL_UINT64(1, out[0].ts_unix_ms);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, out[4].value);

    q.drop(2);
    TEST_ASSERT_EQUAL_UINT(3, q.size());
    n = q.peek(out, 10);
    TEST_ASSERT_EQUAL_UINT64(3, out[0].ts_unix_ms);
}

void test_persists_across_reinit() {
    {
        LittleFsTelemetryQueue q1;
        q1.begin();
        q1.clear();
        q1.append(rec(42, 1.0f));
        q1.append(rec(43, 2.0f));
    }
    // Simulated reboot: fresh instance reads header back.
    LittleFsTelemetryQueue q2;
    q2.begin();
    TEST_ASSERT_EQUAL_UINT(2, q2.size());
    TelemetryRecord out[2];
    q2.peek(out, 2);
    TEST_ASSERT_EQUAL_UINT64(42, out[0].ts_unix_ms);
    TEST_ASSERT_EQUAL_UINT64(43, out[1].ts_unix_ms);
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_begin_then_clear_is_empty);
    RUN_TEST(test_append_size_peek_drop_roundtrip);
    RUN_TEST(test_persists_across_reinit);
    UNITY_END();
}

void loop() {}
