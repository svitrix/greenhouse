#include <unity.h>
#include "MqttCommandRouter.hpp"
#include "fakes/FakeMqttClient.hpp"

using gh::presentation::MqttCommandRouter;
using gh::test::FakeMqttClient;

void test_on_payload_invokes_on_handler(void) {
    FakeMqttClient mqtt;
    int on_calls = 0, off_calls = 0;
    MqttCommandRouter router{
        mqtt,
        "abcdef",
        [&]() { on_calls++; },
        [&]() { off_calls++; }
    };
    router.subscribe();

    mqtt.deliverMessage("greenhouse/abcdef/pump/cmd", "ON");
    TEST_ASSERT_EQUAL(1, on_calls);
    TEST_ASSERT_EQUAL(0, off_calls);
}

void test_off_payload_invokes_off_handler(void) {
    FakeMqttClient mqtt;
    int on_calls = 0, off_calls = 0;
    MqttCommandRouter router{mqtt, "abcdef",
                              [&](){ on_calls++; }, [&](){ off_calls++; }};
    router.subscribe();

    mqtt.deliverMessage("greenhouse/abcdef/pump/cmd", "OFF");
    TEST_ASSERT_EQUAL(0, on_calls);
    TEST_ASSERT_EQUAL(1, off_calls);
}

void test_unknown_payload_is_ignored(void) {
    FakeMqttClient mqtt;
    int on_calls = 0, off_calls = 0;
    MqttCommandRouter router{mqtt, "abcdef",
                              [&](){ on_calls++; }, [&](){ off_calls++; }};
    router.subscribe();

    mqtt.deliverMessage("greenhouse/abcdef/pump/cmd", "LOL");
    TEST_ASSERT_EQUAL(0, on_calls);
    TEST_ASSERT_EQUAL(0, off_calls);
}

void test_subscribe_registers_correct_topic(void) {
    FakeMqttClient mqtt;
    MqttCommandRouter router{mqtt, "abcdef",
                              [](){}, [](){}};
    router.subscribe();
    TEST_ASSERT_EQUAL(1, static_cast<int>(mqtt.subscriptions.size()));
    TEST_ASSERT_EQUAL_STRING("greenhouse/abcdef/pump/cmd",
                             mqtt.subscriptions[0].topic.c_str());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_on_payload_invokes_on_handler);
    RUN_TEST(test_off_payload_invokes_off_handler);
    RUN_TEST(test_unknown_payload_is_ignored);
    RUN_TEST(test_subscribe_registers_correct_topic);
    return UNITY_END();
}
