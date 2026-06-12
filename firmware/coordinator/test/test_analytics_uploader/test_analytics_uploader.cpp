#include <unity.h>
#include <cmath>
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

// C4: the flush gate must stay correct across the 32-bit millis() wrap.
void test_tick_flushes_across_millis_wrap() {
    FakeClock clk; FakeLogger log; FakeHttpClient http; FakeTelemetryQueue q;
    AnalyticsUploader u{q, http, clk, log, make_cfg(60'000)};
    http.reply200();

    // Hop the flush stamp up to just below UINT32_MAX in two steps, each within
    // the signed-diff window, so last_flush_ms_ lands near the rollover boundary.
    clk.now_ms = 0x8000'0000u;
    u.onReading(rec(1, 0, TelemetryKind::AirTemp, 22.0f));
    u.tick();
    TEST_ASSERT_EQUAL_UINT(1, http.calls.size());

    clk.now_ms = 0xFFFF'0000u;
    u.onReading(rec(2, 0, TelemetryKind::AirTemp, 22.0f));
    u.tick();
    TEST_ASSERT_EQUAL_UINT(2, http.calls.size());  // last_flush_ms_ ~ 0xFFFF0000

    // Clock wraps past 0; >60 s elapsed across the boundary. A naive
    // `now < next_allowed` (next_allowed wraps to a tiny value) would still flush
    // by luck, but a 64-bit-widened `last + period` would NOT — the wrap-safe
    // signed diff is what keeps this correct.
    clk.now_ms = 0x0001'0000u;  // wrapped; (now - last) ~ 131 s
    u.onReading(rec(3, 0, TelemetryKind::AirTemp, 22.0f));
    u.tick();
    TEST_ASSERT_EQUAL_UINT(3, http.calls.size());
}

// C5: an oversized batch must NOT be posted truncated. The uploader shrinks the
// batch until it fits and posts a *complete* (valid-JSON) smaller body, making
// forward progress rather than shipping a broken one.
void test_oversized_batch_is_not_posted_truncated() {
    FakeClock clk; FakeLogger log; FakeHttpClient http; FakeTelemetryQueue q;
    AnalyticsUploader u{q, http, clk, log, make_cfg()};
    // 500 records overflow the 32 KB build buffer (~76 B/record ≈ 38 KB).
    for (int i = 0; i < 500; ++i) {
        u.onReading(rec(1700000000000ULL + static_cast<uint64_t>(i), 0,
                        TelemetryKind::AirTemp, 22.5f));
    }
    http.reply200();
    u.flushNow();

    // Exactly one POST, with a complete (non-truncated) JSON body, and the
    // queue made progress (a sub-batch was dropped, leftovers retained).
    TEST_ASSERT_EQUAL_UINT(1, http.calls.size());
    const auto& body = http.calls[0].body;
    TEST_ASSERT_NOT_NULL(std::strstr(body.c_str(), "],\"events\":[]}"));  // closed
    TEST_ASSERT_TRUE(body.size() <= 32u * 1024u);                         // fit buffer
    TEST_ASSERT_TRUE(q.size() < 500);                                     // progress
    TEST_ASSERT_TRUE(q.size() > 0);                                       // shrunk, not all
}

// C5: a non-finite value must not corrupt the JSON body.
void test_nan_value_does_not_corrupt_body() {
    FakeClock clk; FakeLogger log; FakeHttpClient http; FakeTelemetryQueue q;
    AnalyticsUploader u{q, http, clk, log, make_cfg()};
    const float nan_v = std::nanf("");
    u.onReading(rec(1700000000000ULL, 0, TelemetryKind::AirTemp, nan_v));
    http.reply200();
    u.flushNow();

    TEST_ASSERT_EQUAL_UINT(1, http.calls.size());
    const auto& body = http.calls[0].body;
    // No bare nan/inf token leaked; the value is rendered as JSON null.
    TEST_ASSERT_NULL(std::strstr(body.c_str(), "nan"));
    TEST_ASSERT_NULL(std::strstr(body.c_str(), "inf"));
    TEST_ASSERT_NOT_NULL(std::strstr(body.c_str(), "\"value\":null"));
}

// C6: flushNow must honour an active backoff so a manual trigger cannot hammer
// a hub that asked us to back off.
void test_flush_now_honours_active_backoff() {
    FakeClock clk; FakeLogger log; FakeHttpClient http; FakeTelemetryQueue q;
    AnalyticsUploader u{q, http, clk, log, make_cfg(1'000)};
    u.onReading(rec(1, 0, TelemetryKind::AirTemp, 22.0f));

    // Engage backoff via a 5xx.
    http.reply500();
    clk.now_ms = 1'001;
    u.tick();
    TEST_ASSERT_EQUAL_UINT(1, http.calls.size());

    // Manual trigger inside the backoff window must be suppressed.
    http.reply200();
    clk.now_ms = 5'000;  // backoff is >= 60s, still active
    u.flushNow();
    TEST_ASSERT_EQUAL_UINT(1, http.calls.size());

    // Once the backoff window elapses, the manual trigger goes through.
    clk.now_ms = 70'000;
    u.flushNow();
    TEST_ASSERT_EQUAL_UINT(2, http.calls.size());
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
    RUN_TEST(test_tick_flushes_across_millis_wrap);
    RUN_TEST(test_oversized_batch_is_not_posted_truncated);
    RUN_TEST(test_nan_value_does_not_corrupt_body);
    RUN_TEST(test_flush_now_honours_active_backoff);
    return UNITY_END();
}
