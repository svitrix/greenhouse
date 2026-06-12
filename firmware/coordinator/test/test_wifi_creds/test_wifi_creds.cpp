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

void test_default_schema_version_is_current() {
    WifiCreds c{};
    TEST_ASSERT_EQUAL_UINT8(gh::domain::kWifiCredsSchemaVersion, c.schema_version);
}

// Simulates a corrupt / truncated NVS record: char[] fields with no NUL inside
// their bounds. valid() must reject it instead of letting strlen over-read.
void test_invalid_when_no_nul_terminator() {
    WifiCreds c{};
    std::memset(c.ssid, 'A', sizeof(c.ssid));
    std::memset(c.password, 'B', sizeof(c.password));
    std::memset(c.hostname, 'C', sizeof(c.hostname));
    TEST_ASSERT_FALSE(c.valid());
}

// normalizeForStorage() restores the terminator so the record becomes safe.
void test_normalize_forces_nul_and_passes_valid() {
    WifiCreds c{};
    std::memset(c.ssid, 'A', sizeof(c.ssid));
    std::memset(c.password, 'B', sizeof(c.password));
    std::memset(c.hostname, 0, sizeof(c.hostname));
    c.normalizeForStorage();
    TEST_ASSERT_TRUE(c.valid());
    TEST_ASSERT_EQUAL_UINT32(32, static_cast<uint32_t>(std::strlen(c.ssid)));
    TEST_ASSERT_EQUAL_UINT32(64, static_cast<uint32_t>(std::strlen(c.password)));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_valid_with_ssid);
    RUN_TEST(test_invalid_empty_ssid);
    RUN_TEST(test_valid_max_length_ssid);
    RUN_TEST(test_valid_no_password_open_wifi);
    RUN_TEST(test_default_schema_version_is_current);
    RUN_TEST(test_invalid_when_no_nul_terminator);
    RUN_TEST(test_normalize_forces_nul_and_passes_valid);
    return UNITY_END();
}
