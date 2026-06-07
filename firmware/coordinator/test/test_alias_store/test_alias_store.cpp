#include <unity.h>
#include "fakes/InMemoryAliasStore.hpp"

using gh::domain::ErrorCode;
using gh::domain::NodeId;
using gh::test::InMemoryAliasStore;

void test_set_and_load_alias(void) {
    InMemoryAliasStore s;
    TEST_ASSERT_EQUAL(ErrorCode::Ok, s.setAlias(NodeId{1}, "Tomatoes"));
    auto a = s.alias(NodeId{1});
    TEST_ASSERT_TRUE(a.has_value());
    TEST_ASSERT_EQUAL_STRING("Tomatoes", a->data());
}

void test_reject_too_long_alias(void) {
    InMemoryAliasStore s;
    TEST_ASSERT_EQUAL(ErrorCode::AliasTooLong,
                      s.setAlias(NodeId{1},
                          "12345678901234567890ABCD"));   // 24 chars
}

void test_clear_alias(void) {
    InMemoryAliasStore s;
    (void)s.setAlias(NodeId{1}, "x");
    TEST_ASSERT_EQUAL(ErrorCode::Ok, s.clearAlias(NodeId{1}));
    TEST_ASSERT_FALSE(s.alias(NodeId{1}).has_value());
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_set_and_load_alias);
    RUN_TEST(test_reject_too_long_alias);
    RUN_TEST(test_clear_alias);
    return UNITY_END();
}
