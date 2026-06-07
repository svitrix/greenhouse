#include <Arduino.h>
#include <unity.h>
#include <Preferences.h>
#include <cstring>
#include "persistence/NvsWifiCredsStore.hpp"

using gh::domain::ErrorCode;
using gh::domain::WifiCreds;
using gh::infra::NvsWifiCredsStore;

namespace {
void clearNamespace() {
    Preferences p;
    p.begin("wifi", false);
    p.clear();
    p.end();
}
WifiCreds make(const char* ssid, const char* pw, const char* host = "") {
    WifiCreds c{};
    std::strncpy(c.ssid, ssid, sizeof(c.ssid) - 1);
    std::strncpy(c.password, pw, sizeof(c.password) - 1);
    std::strncpy(c.hostname, host, sizeof(c.hostname) - 1);
    return c;
}
}

void setUp() { clearNamespace(); }
void tearDown() { clearNamespace(); }

void test_load_on_empty_returns_ConfigNotFound() {
    NvsWifiCredsStore s;
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(s.begin()));
    auto loaded = s.load();
    TEST_ASSERT_FALSE(loaded.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::ConfigNotFound),
                      static_cast<int>(loaded.err));
}

void test_save_then_load_returns_same() {
    NvsWifiCredsStore s;
    s.begin();
    auto c = make("MyHomeWifi", "secret123", "greenhouse");
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(s.save(c)));
    auto loaded = s.load();
    TEST_ASSERT_TRUE(loaded.ok());
    TEST_ASSERT_EQUAL_STRING("MyHomeWifi", loaded.value.ssid);
    TEST_ASSERT_EQUAL_STRING("secret123", loaded.value.password);
    TEST_ASSERT_EQUAL_STRING("greenhouse", loaded.value.hostname);
}

void test_save_invalid_returns_SensorOutOfRange() {
    NvsWifiCredsStore s;
    s.begin();
    WifiCreds bad{};   // empty ssid
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::SensorOutOfRange),
                      static_cast<int>(s.save(bad)));
}

void test_save_overwrites() {
    NvsWifiCredsStore s;
    s.begin();
    s.save(make("First", "p1"));
    s.save(make("Second", "p2"));
    auto loaded = s.load();
    TEST_ASSERT_TRUE(loaded.ok());
    TEST_ASSERT_EQUAL_STRING("Second", loaded.value.ssid);
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_load_on_empty_returns_ConfigNotFound);
    RUN_TEST(test_save_then_load_returns_same);
    RUN_TEST(test_save_invalid_returns_SensorOutOfRange);
    RUN_TEST(test_save_overwrites);
    UNITY_END();
}
void loop() {}
