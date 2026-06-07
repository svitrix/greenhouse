#include <Arduino.h>
#include <WiFi.h>
#include <unity.h>
#include <cstring>
#include "EspHttpsClient.hpp"
#include "secrets.hpp"  // WIFI_SSID, WIFI_PASS, BACKEND_URL, BACKEND_KEY

using namespace gh::infra;
using namespace gh::domain;

void test_post_returns_200_on_known_endpoint() {
    EspHttpsClient client;  // setInsecure() — local dev backend
    const char* body =
        "{\"device_id\":\"gh-hwtest\",\"fw_version\":\"0.1.0\","
        "\"batch_id\":\"hw-1\","
        "\"readings\":[{\"ts\":1700000000000,\"channel_id\":0,"
        "\"kind\":\"air_temp\",\"value\":21.0,\"status\":0}],"
        "\"events\":[]}";
    auto resp = client.postJson(BACKEND_URL, BACKEND_KEY,
                                body, std::strlen(body), 10'000);
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(resp.error));
    // 200 first time, 409 on a re-run because of unique-key dedup is not
    // surfaced in MVP — 200 with duplicates_skipped is what the backend
    // actually returns. So either way: < 300.
    TEST_ASSERT_TRUE_MESSAGE(resp.http_status >= 200 && resp.http_status < 300,
                             "expected 2xx from backend");
}

void setup() {
    delay(2000);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) delay(500);

    UNITY_BEGIN();
    RUN_TEST(test_post_returns_200_on_known_endpoint);
    UNITY_END();
}

void loop() {}
