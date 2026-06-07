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

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_valid_minimal);
    RUN_TEST(test_invalid_empty_host);
    RUN_TEST(test_invalid_zero_port);
    return UNITY_END();
}
