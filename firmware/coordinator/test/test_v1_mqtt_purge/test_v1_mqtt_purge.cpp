#include <unity.h>
#include "telemetry/V1MqttPurge.hpp"
#include "fakes/FakeMqttClient.hpp"
#include "fakes/FakeOneShotFlagStore.hpp"

using gh::app::V1MqttPurge;
using gh::test::FakeMqttClient;
using gh::test::FakeOneShotFlagStore;

static constexpr const char* kDeviceId = "a1b2c3";
static constexpr int kV1TopicCount = 11;

void test_purge_publishes_and_sets_flag_once(void) {
    FakeMqttClient mqtt;
    FakeOneShotFlagStore flags;
    V1MqttPurge purge{flags};

    TEST_ASSERT_TRUE(purge.runIfNeeded(mqtt, kDeviceId));
    TEST_ASSERT_EQUAL_INT(kV1TopicCount, static_cast<int>(mqtt.published.size()));
    for (const auto& msg : mqtt.published) {
        TEST_ASSERT_TRUE(msg.retain);
        TEST_ASSERT_TRUE(msg.payload.empty());
    }
    TEST_ASSERT_EQUAL_INT(1, flags.set_calls);
    TEST_ASSERT_TRUE(flags.isSet("mqtt_purge_v1"));
}

void test_purge_is_one_shot(void) {
    FakeMqttClient mqtt;
    FakeOneShotFlagStore flags;
    V1MqttPurge purge{flags};

    TEST_ASSERT_TRUE(purge.runIfNeeded(mqtt, kDeviceId));
    mqtt.published.clear();

    // Second pass: flag already set → no republish, no extra set() call.
    TEST_ASSERT_TRUE(purge.runIfNeeded(mqtt, kDeviceId));
    TEST_ASSERT_EQUAL_INT(0, static_cast<int>(mqtt.published.size()));
    TEST_ASSERT_EQUAL_INT(1, flags.set_calls);
}

void test_purge_skips_when_disconnected(void) {
    FakeMqttClient mqtt;
    mqtt.connected = false;
    FakeOneShotFlagStore flags;
    V1MqttPurge purge{flags};

    TEST_ASSERT_FALSE(purge.runIfNeeded(mqtt, kDeviceId));
    TEST_ASSERT_EQUAL_INT(0, static_cast<int>(mqtt.published.size()));
    TEST_ASSERT_EQUAL_INT(0, flags.set_calls);
    TEST_ASSERT_FALSE(flags.isSet("mqtt_purge_v1"));
}

void test_purge_retries_when_flag_write_fails(void) {
    FakeMqttClient mqtt;
    FakeOneShotFlagStore flags;
    flags.fail_on_set = true;
    V1MqttPurge purge{flags};

    // Flag write fails → runIfNeeded reports incomplete so the caller retries.
    TEST_ASSERT_FALSE(purge.runIfNeeded(mqtt, kDeviceId));
    TEST_ASSERT_EQUAL_INT(kV1TopicCount, static_cast<int>(mqtt.published.size()));
    TEST_ASSERT_FALSE(flags.isSet("mqtt_purge_v1"));

    // Recover: next pass succeeds and publishes exactly once more.
    flags.fail_on_set = false;
    mqtt.published.clear();
    TEST_ASSERT_TRUE(purge.runIfNeeded(mqtt, kDeviceId));
    TEST_ASSERT_EQUAL_INT(kV1TopicCount, static_cast<int>(mqtt.published.size()));
    TEST_ASSERT_TRUE(flags.isSet("mqtt_purge_v1"));
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_purge_publishes_and_sets_flag_once);
    RUN_TEST(test_purge_is_one_shot);
    RUN_TEST(test_purge_skips_when_disconnected);
    RUN_TEST(test_purge_retries_when_flag_write_fails);
    return UNITY_END();
}
