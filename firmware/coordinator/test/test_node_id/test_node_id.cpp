#include <unity.h>
#include <cstring>
#include "entities/NodeId.hpp"

using gh::domain::NodeId;

void test_toHex16_emits_uppercase_zero_padded(void) {
    NodeId id{0x00124B001A2B3C4Dull};
    const auto buf = id.toHex16();
    TEST_ASSERT_EQUAL_STRING("00124B001A2B3C4D", buf.data());
}

void test_toHex16_emits_zero(void) {
    NodeId id{0};
    const auto buf = id.toHex16();
    TEST_ASSERT_EQUAL_STRING("0000000000000000", buf.data());
}

void test_parseHex16_round_trips(void) {
    auto parsed = NodeId::parseHex16("00124b001a2b3c4d");   // lowercase tolerated
    TEST_ASSERT_TRUE(parsed.has_value());
    TEST_ASSERT_EQUAL_UINT64(0x00124B001A2B3C4Dull, parsed->ieee);
}

void test_parseHex16_rejects_short_input(void) {
    TEST_ASSERT_FALSE(NodeId::parseHex16("00124B").has_value());
    TEST_ASSERT_FALSE(NodeId::parseHex16("").has_value());
}

void test_parseHex16_rejects_non_hex(void) {
    TEST_ASSERT_FALSE(NodeId::parseHex16("00124B001A2B3C4Z").has_value());
}

void test_equality_and_ordering(void) {
    NodeId a{0x10};
    NodeId b{0x10};
    NodeId c{0x20};
    TEST_ASSERT_TRUE(a == b);
    TEST_ASSERT_TRUE(a < c);
    TEST_ASSERT_FALSE(c < a);
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_toHex16_emits_uppercase_zero_padded);
    RUN_TEST(test_toHex16_emits_zero);
    RUN_TEST(test_parseHex16_round_trips);
    RUN_TEST(test_parseHex16_rejects_short_input);
    RUN_TEST(test_parseHex16_rejects_non_hex);
    RUN_TEST(test_equality_and_ordering);
    return UNITY_END();
}
