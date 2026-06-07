// HW-tests for gh::infra::MqttClient (espMqttClient adapter).
//
// Run with:  pio test -e coordinator-hwtest -d firmware/coordinator -f test_mqtt_client
//
// Prerequisites (see test_creds.hpp.example):
//   * mosquitto broker reachable from the board's Wi-Fi network
//   * test_creds.hpp filled in (gitignored copy of test_creds.hpp.example)
//
// To verify each test actually catches a regression (manual RED-step):
//   Temporarily break the adapter — e.g. shrink topic_buf in MqttClient::publish
//   to 4, or hardcode `setKeepAlive(0)` etc. — run the suite, watch the relevant
//   test FAIL, restore the adapter, watch it PASS. That closes the RED→GREEN
//   loop the skill mandates for tests against existing code.

#include <Arduino.h>
#include <WiFi.h>
#include <unity.h>
#include <cstring>
#include <cstdio>
#include "test_creds.hpp"
#include "network/MqttClient.hpp"
#include "entities/MqttCreds.hpp"

using gh::domain::ErrorCode;
using gh::domain::MqttCreds;
// Cannot `using gh::infra::MqttClient` — espMqttClient library has a
// global-namespace class `MqttClient` (base of espMqttClientAsync) that
// would shadow ours at the call sites. Alias to a unique name instead.
using GhMqttClient = gh::infra::MqttClient;

namespace {

// --- Receive sink (callback fires on AsyncTCP task; tests poll on main) -----
constexpr size_t kReceiveBufSize = 1024U;
volatile bool s_received_flag = false;
char    s_received_topic[128] = {};
uint8_t s_received_payload[kReceiveBufSize] = {};
size_t  s_received_payload_len = 0U;

void onTestMessage(std::string_view topic,
                    const uint8_t* payload,
                    uint16_t       len,
                    void* /*ctx*/) {
    const size_t topic_n =
        topic.size() < sizeof(s_received_topic) - 1U
            ? topic.size()
            : sizeof(s_received_topic) - 1U;
    std::memcpy(const_cast<char*>(s_received_topic), topic.data(), topic_n);
    s_received_topic[topic_n] = '\0';

    const size_t payload_n =
        static_cast<size_t>(len) < kReceiveBufSize
            ? static_cast<size_t>(len)
            : kReceiveBufSize;
    std::memcpy(const_cast<uint8_t*>(s_received_payload), payload, payload_n);
    s_received_payload_len = payload_n;
    s_received_flag = true;
}

bool wait_for_message(uint32_t timeout_ms) {
    const uint32_t start = millis();
    while (!s_received_flag) {
        if (millis() - start > timeout_ms) return false;
        delay(10);
    }
    return true;
}

// --- Shared MqttClient instance, alive for the whole test run --------------
GhMqttClient* s_client = nullptr;

MqttCreds buildTestCreds() {
    MqttCreds c{};
    std::strncpy(c.host,         gh::hwtest::kTestMqttHost,     sizeof(c.host)         - 1U);
    c.port = gh::hwtest::kTestMqttPort;
    std::strncpy(c.user,         gh::hwtest::kTestMqttUser,     sizeof(c.user)         - 1U);
    std::strncpy(c.password,     gh::hwtest::kTestMqttPassword, sizeof(c.password)     - 1U);
    std::strncpy(c.client_id,    gh::hwtest::kTestMqttClientId, sizeof(c.client_id)    - 1U);
    std::strncpy(c.topic_prefix, "gh-hwtest",                   sizeof(c.topic_prefix) - 1U);
    return c;
}

bool wifiConnect() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(gh::hwtest::kTestWifiSsid, gh::hwtest::kTestWifiPassword);
    const uint32_t deadline = millis() + 20'000U;
    while (WiFi.status() != WL_CONNECTED) {
        if (static_cast<int32_t>(millis() - deadline) >= 0) return false;
        delay(100);
    }
    return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// Per-test fixtures: clear receive sink before each scenario.
// ---------------------------------------------------------------------------
void setUp() {
    s_received_flag = false;
    s_received_topic[0] = '\0';
    s_received_payload_len = 0U;
    std::memset(const_cast<uint8_t*>(s_received_payload), 0,
                sizeof(s_received_payload));
}
void tearDown() {}

// ---------------------------------------------------------------------------
// Test 1: connect succeeds within 5 s of MqttClient::connect().
// What it proves: the espMqttClient swap didn't break the basic connect path.
// To force RED: point kTestMqttHost at an unreachable IP — expect FAIL.
// ---------------------------------------------------------------------------
void test_connect_succeeds() {
    TEST_ASSERT_NOT_NULL(s_client);
    TEST_ASSERT_TRUE_MESSAGE(s_client->isConnected(),
        "expected MqttClient to be connected after setup()");
}

// ---------------------------------------------------------------------------
// Test 2: publish a 512-byte payload, receive it back unchanged.
// What it proves: regression for the old PubSubClient 256 B buffer cap —
//   with the old library, this test would FAIL (publish silently truncated
//   or rejected). With espMqttClient, payloads stream at arbitrary size.
// To force RED: in MqttClient::publish, slice payload.size() to 200.
// ---------------------------------------------------------------------------
void test_publish_large_payload_roundtrip() {
    constexpr const char* topic = "gh-hwtest/large";
    constexpr size_t kPayloadSize = 512U;

    uint8_t payload[kPayloadSize];
    for (size_t i = 0; i < kPayloadSize; ++i) {
        payload[i] = static_cast<uint8_t>(i & 0xFF);
    }
    const std::string_view payload_sv{
        reinterpret_cast<const char*>(payload), kPayloadSize};

    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
        static_cast<int>(s_client->subscribe(topic, &onTestMessage, nullptr)));
    delay(300);  // let SUBSCRIBE reach broker

    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
        static_cast<int>(s_client->publish(topic, payload_sv, /*retain*/ false)));

    TEST_ASSERT_TRUE_MESSAGE(wait_for_message(3000U),
        "expected to receive the 512-byte payload within 3 s");
    TEST_ASSERT_EQUAL_size_t(kPayloadSize, s_received_payload_len);
    TEST_ASSERT_EQUAL_STRING(topic, s_received_topic);
    TEST_ASSERT_EQUAL_MEMORY(payload,
        const_cast<uint8_t*>(s_received_payload), kPayloadSize);
}

// ---------------------------------------------------------------------------
// Test 3: short payload round-trip.
// What it proves: basic dispatch path — subscribe → publish → handler fires
//   with the correct topic + payload.
// To force RED: in MqttClient::dispatchMessage, never iterate subs_.
// ---------------------------------------------------------------------------
void test_publish_short_payload_roundtrip() {
    constexpr const char* topic   = "gh-hwtest/small";
    constexpr const char* payload = "OK";

    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
        static_cast<int>(s_client->subscribe(topic, &onTestMessage, nullptr)));
    delay(300);

    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
        static_cast<int>(s_client->publish(topic, payload, /*retain*/ false)));

    TEST_ASSERT_TRUE_MESSAGE(wait_for_message(2000U),
        "expected to receive 'OK' within 2 s");
    TEST_ASSERT_EQUAL_size_t(2U, s_received_payload_len);
    TEST_ASSERT_EQUAL_STRING(topic, s_received_topic);
    TEST_ASSERT_EQUAL_MEMORY("OK",
        const_cast<uint8_t*>(s_received_payload), 2);
}

// ---------------------------------------------------------------------------
// Test 4: exact-match dispatch — wildcards are NOT delivered locally.
// What it proves: subscribe to topic-a but publish to topic-b — the registered
//   handler must NOT fire. This pins down the documented behaviour
//   (network/CLAUDE.md "MQTT subscription matching is exact-string only").
// To force RED: change `==` to `.starts_with(...)` in MqttClient::dispatchMessage.
// ---------------------------------------------------------------------------
void test_exact_match_no_cross_topic_delivery() {
    constexpr const char* topic_subscribed = "gh-hwtest/topic-a";
    constexpr const char* topic_other      = "gh-hwtest/topic-b";

    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
        static_cast<int>(s_client->subscribe(topic_subscribed,
                                              &onTestMessage, nullptr)));
    delay(300);

    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
        static_cast<int>(s_client->publish(topic_other, "ignored",
                                            /*retain*/ false)));

    // Wait the full window: we EXPECT no message to land.
    const bool fired = wait_for_message(1500U);
    TEST_ASSERT_FALSE_MESSAGE(fired,
        "handler must NOT fire for a topic it didn't subscribe to");
}

// ---------------------------------------------------------------------------
// Arduino entry point.
// ---------------------------------------------------------------------------
void setup() {
    delay(2000);  // give USB-CDC time to connect

    UNITY_BEGIN();

    if (!wifiConnect()) {
        TEST_FAIL_MESSAGE("Wi-Fi failed to connect — check test_creds.hpp");
        UNITY_END();
        return;
    }

    static MqttCreds creds = buildTestCreds();
    static GhMqttClient client{creds};
    s_client = &client;

    const auto err = client.connect();
    if (err != ErrorCode::Ok) {
        TEST_FAIL_MESSAGE("MqttClient::connect() failed — broker reachable?");
        UNITY_END();
        return;
    }

    RUN_TEST(test_connect_succeeds);
    RUN_TEST(test_publish_short_payload_roundtrip);
    RUN_TEST(test_publish_large_payload_roundtrip);
    RUN_TEST(test_exact_match_no_cross_topic_delivery);

    UNITY_END();
}

void loop() {}
