#include <unity.h>
#include <cstring>
#include "AnalyticsUploader.hpp"
#include "../fakes/FakeClock.hpp"
#include "../fakes/FakeLogger.hpp"
#include "../fakes/FakeHttpClient.hpp"
#include "../fakes/FakeTelemetryQueue.hpp"

using namespace gh::app;
using namespace gh::domain;
using namespace gh::test;

static AnalyticsUploaderConfig make_cfg(uint32_t flush_ms = 60'000) {
    return AnalyticsUploaderConfig{
        /*backend_url*/    "http://localhost:8000/ingest",
        /*api_key*/        "dev-secret-key",
        /*device_id*/      "gh-test",
        /*fw_version*/     "0.1.0",
        /*flush_period_ms*/flush_ms,
    };
}

static TelemetryRecord rec(uint64_t ts, uint8_t ch, TelemetryKind k, float v) {
    return TelemetryRecord{ts, ch, k, v, kTelemetryRawNotApplicable, 0};
}

void test_records_are_buffered_via_queue() {
    FakeClock clk; FakeLogger log; FakeHttpClient http; FakeTelemetryQueue q;
    AnalyticsUploader u{q, http, clk, log, make_cfg()};
    u.onReading(rec(1, 0, TelemetryKind::AirTemp, 22.0f));
    u.onReading(rec(2, 0, TelemetryKind::AirHumidity, 60.0f));
    TEST_ASSERT_EQUAL_UINT(2, q.size());
}

void test_flush_drains_queue_on_200() {
    FakeClock clk; FakeLogger log; FakeHttpClient http; FakeTelemetryQueue q;
    AnalyticsUploader u{q, http, clk, log, make_cfg()};
    u.onReading(rec(1, 0, TelemetryKind::AirTemp, 22.0f));
    http.reply200();
    u.flushNow();
    TEST_ASSERT_EQUAL_UINT(0, q.size());
    TEST_ASSERT_EQUAL_UINT(1, http.calls.size());
}

void test_flush_keeps_records_on_5xx() {
    FakeClock clk; FakeLogger log; FakeHttpClient http; FakeTelemetryQueue q;
    AnalyticsUploader u{q, http, clk, log, make_cfg()};
    u.onReading(rec(1, 0, TelemetryKind::AirTemp, 22.0f));
    http.reply500();
    u.flushNow();
    TEST_ASSERT_EQUAL_UINT(1, q.size());
}

void test_flush_drops_records_on_4xx() {
    FakeClock clk; FakeLogger log; FakeHttpClient http; FakeTelemetryQueue q;
    AnalyticsUploader u{q, http, clk, log, make_cfg()};
    u.onReading(rec(1, 0, TelemetryKind::AirTemp, 22.0f));
    http.reply400();
    u.flushNow();
    TEST_ASSERT_EQUAL_UINT(0, q.size());
    TEST_ASSERT_EQUAL_UINT(1, u.poison4xxCount());
}

void test_tick_only_flushes_after_period() {
    FakeClock clk; FakeLogger log; FakeHttpClient http; FakeTelemetryQueue q;
    AnalyticsUploader u{q, http, clk, log, make_cfg(60'000)};
    u.onReading(rec(1, 0, TelemetryKind::AirTemp, 22.0f));
    http.reply200();

    clk.now_ms = 30'000;
    u.tick();
    TEST_ASSERT_EQUAL_UINT(0, http.calls.size());

    clk.now_ms = 60'001;
    u.tick();
    TEST_ASSERT_EQUAL_UINT(1, http.calls.size());
}

void test_exponential_backoff_after_failure() {
    FakeClock clk; FakeLogger log; FakeHttpClient http; FakeTelemetryQueue q;
    AnalyticsUploader u{q, http, clk, log, make_cfg(1'000)};
    u.onReading(rec(1, 0, TelemetryKind::AirTemp, 22.0f));

    http.reply500();
    clk.now_ms = 1'001;
    u.tick();
    TEST_ASSERT_EQUAL_UINT(1, http.calls.size());

    // Backoff is at least 60s — a tick at +30s should NOT flush again
    clk.now_ms = 31'001;
    u.tick();
    TEST_ASSERT_EQUAL_UINT(1, http.calls.size());

    // After 60s of backoff, the next tick reaches the gate and flushes
    clk.now_ms = 61'002;
    u.tick();
    TEST_ASSERT_EQUAL_UINT(2, http.calls.size());
}

void test_body_contains_device_id_and_records() {
    FakeClock clk; FakeLogger log; FakeHttpClient http; FakeTelemetryQueue q;
    AnalyticsUploader u{q, http, clk, log, make_cfg()};
    u.onReading(rec(1700000000000ULL, 0, TelemetryKind::AirTemp, 22.5f));
    http.reply200();
    u.flushNow();

    TEST_ASSERT_EQUAL_UINT(1, http.calls.size());
    const auto& body = http.calls[0].body;
    TEST_ASSERT_NOT_NULL(std::strstr(body.c_str(), "\"device_id\":\"gh-test\""));
    TEST_ASSERT_NOT_NULL(std::strstr(body.c_str(), "\"kind\":\"air_temp\""));
    TEST_ASSERT_NOT_NULL(std::strstr(body.c_str(), "\"value\":22.5"));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_records_are_buffered_via_queue);
    RUN_TEST(test_flush_drains_queue_on_200);
    RUN_TEST(test_flush_keeps_records_on_5xx);
    RUN_TEST(test_flush_drops_records_on_4xx);
    RUN_TEST(test_tick_only_flushes_after_period);
    RUN_TEST(test_exponential_backoff_after_failure);
    RUN_TEST(test_body_contains_device_id_and_records);
    return UNITY_END();
}
