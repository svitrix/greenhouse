#include <unity.h>
#include <cstring>
#include "entities/WifiCreds.hpp"

using gh::domain::WifiCreds;

WifiCreds make(const char* ssid, const char* pw = "p", const char* host = "") {
    WifiCreds c{};
    std::strncpy(c.ssid, ssid, sizeof(c.ssid) - 1);
    std::strncpy(c.password, pw, sizeof(c.password) - 1);
    std::strncpy(c.hostname, host, sizeof(c.hostname) - 1);
    return c;
}

void test_valid_with_ssid() {
    TEST_ASSERT_TRUE(make("MyWifi").valid());
}
void test_invalid_empty_ssid() {
    TEST_ASSERT_FALSE(make("").valid());
}
void test_valid_max_length_ssid() {
    char ssid[33] = {};
    std::memset(ssid, 'A', 32);
    auto c = make(ssid);
    TEST_ASSERT_TRUE(c.valid());
}
void test_valid_no_password_open_wifi() {
    auto c = make("OpenWifi", "");
    TEST_ASSERT_TRUE(c.valid());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_valid_with_ssid);
    RUN_TEST(test_invalid_empty_ssid);
    RUN_TEST(test_valid_max_length_ssid);
    RUN_TEST(test_valid_no_password_open_wifi);
    return UNITY_END();
}
