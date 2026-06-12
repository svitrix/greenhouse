#include <unity.h>
#include <cstring>
#include "entities/MqttCreds.hpp"

using gh::domain::MqttCreds;

MqttCreds make(const char* host, uint16_t port = 1883) {
    MqttCreds c{};
    std::strncpy(c.host, host, sizeof(c.host) - 1);
    c.port = port;
    return c;
}

void test_valid_minimal() {
    TEST_ASSERT_TRUE(make("broker.local", 1883).valid());
}
void test_invalid_empty_host() {
    TEST_ASSERT_FALSE(make("", 1883).valid());
}
void test_invalid_zero_port() {
    TEST_ASSERT_FALSE(make("broker", 0).valid());
}

void test_default_schema_version_is_current() {
    MqttCreds c{};
    TEST_ASSERT_EQUAL_UINT8(gh::domain::kMqttCredsSchemaVersion, c.schema_version);
}

// Simulates a corrupt / truncated NVS record: host has no NUL in bounds.
void test_invalid_when_host_has_no_nul_terminator() {
    MqttCreds c{};
    std::memset(c.host, 'h', sizeof(c.host));   // no terminator
    c.port = 1883;
    TEST_ASSERT_FALSE(c.valid());
}

void test_normalize_forces_nul_and_passes_valid() {
    MqttCreds c{};
    std::memset(c.host, 'h', sizeof(c.host));
    c.port = 1883;
    c.normalizeForStorage();
    TEST_ASSERT_TRUE(c.valid());
    TEST_ASSERT_EQUAL_UINT32(63, static_cast<uint32_t>(std::strlen(c.host)));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_valid_minimal);
    RUN_TEST(test_invalid_empty_host);
    RUN_TEST(test_invalid_zero_port);
    RUN_TEST(test_default_schema_version_is_current);
    RUN_TEST(test_invalid_when_host_has_no_nul_terminator);
    RUN_TEST(test_normalize_forces_nul_and_passes_valid);
    return UNITY_END();
}
