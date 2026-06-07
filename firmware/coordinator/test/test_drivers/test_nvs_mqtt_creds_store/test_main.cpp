#include <Arduino.h>
#include <unity.h>
#include <Preferences.h>
#include <cstring>
#include "persistence/NvsMqttCredsStore.hpp"

using gh::domain::ErrorCode;
using gh::domain::MqttCreds;
using gh::infra::NvsMqttCredsStore;

namespace {
void clearNamespace() {
    Preferences p;
    p.begin("mqtt", false);
    p.clear();
    p.end();
}
MqttCreds make(const char* host, uint16_t port = 1883,
               const char* user = "", const char* pw = "",
               const char* cid = "greenhouse",
               const char* prefix = "greenhouse") {
    MqttCreds c{};
    std::strncpy(c.host, host, sizeof(c.host) - 1);
    c.port = port;
    std::strncpy(c.user, user, sizeof(c.user) - 1);
    std::strncpy(c.password, pw, sizeof(c.password) - 1);
    std::strncpy(c.client_id, cid, sizeof(c.client_id) - 1);
    std::strncpy(c.topic_prefix, prefix, sizeof(c.topic_prefix) - 1);
    return c;
}
}

void setUp() { clearNamespace(); }
void tearDown() { clearNamespace(); }

void test_load_on_empty_returns_ConfigNotFound() {
    NvsMqttCredsStore s;
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(s.begin()));
    auto loaded = s.load();
    TEST_ASSERT_FALSE(loaded.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::ConfigNotFound),
                      static_cast<int>(loaded.err));
}

void test_save_then_load_preserves_host_port_user() {
    NvsMqttCredsStore s;
    s.begin();
    auto c = make("broker.local", 8883, "alice", "wonderland");
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(s.save(c)));
    auto loaded = s.load();
    TEST_ASSERT_TRUE(loaded.ok());
    TEST_ASSERT_EQUAL_STRING("broker.local", loaded.value.host);
    TEST_ASSERT_EQUAL_UINT16(8883, loaded.value.port);
    TEST_ASSERT_EQUAL_STRING("alice", loaded.value.user);
}

void test_save_invalid_returns_SensorOutOfRange() {
    NvsMqttCredsStore s;
    s.begin();
    MqttCreds bad{};   // empty host, port=0
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::SensorOutOfRange),
                      static_cast<int>(s.save(bad)));
}

void test_save_overwrites() {
    NvsMqttCredsStore s;
    s.begin();
    s.save(make("first.broker", 1883));
    s.save(make("second.broker", 1884));
    auto loaded = s.load();
    TEST_ASSERT_TRUE(loaded.ok());
    TEST_ASSERT_EQUAL_STRING("second.broker", loaded.value.host);
    TEST_ASSERT_EQUAL_UINT16(1884, loaded.value.port);
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_load_on_empty_returns_ConfigNotFound);
    RUN_TEST(test_save_then_load_preserves_host_port_user);
    RUN_TEST(test_save_invalid_returns_SensorOutOfRange);
    RUN_TEST(test_save_overwrites);
    UNITY_END();
}
void loop() {}
