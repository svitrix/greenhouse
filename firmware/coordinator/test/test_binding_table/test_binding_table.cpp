#include <unity.h>
#include "network/ZigbeeBindingTable.hpp"

using gh::domain::NodeId;
using gh::infra::ZigbeeBindingTable;

void test_announce_then_resolve(void) {
    ZigbeeBindingTable t;
    t.onDeviceAnnounced(0x1234, 0xAABBCCDDEEFF0011ull);
    auto r = t.resolve(0x1234);
    TEST_ASSERT_TRUE(r.has_value());
    TEST_ASSERT_EQUAL_UINT64(0xAABBCCDDEEFF0011ull, r->ieee);
}

void test_resolve_unknown_returns_nullopt(void) {
    ZigbeeBindingTable t;
    TEST_ASSERT_FALSE(t.resolve(0x9999).has_value());
}

void test_left_removes_mapping(void) {
    ZigbeeBindingTable t;
    t.onDeviceAnnounced(0x1234, 0xAAAAull);
    t.onDeviceLeft(0x1234);
    TEST_ASSERT_FALSE(t.resolve(0x1234).has_value());
}

void test_reannounce_updates_short_addr(void) {
    ZigbeeBindingTable t;
    t.onDeviceAnnounced(0x1234, 0xAAAAull);
    t.onDeviceAnnounced(0x5678, 0xAAAAull);
    TEST_ASSERT_FALSE(t.resolve(0x1234).has_value());
    auto r = t.resolve(0x5678);
    TEST_ASSERT_TRUE(r.has_value());
    TEST_ASSERT_EQUAL_UINT64(0xAAAAull, r->ieee);
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_announce_then_resolve);
    RUN_TEST(test_resolve_unknown_returns_nullopt);
    RUN_TEST(test_left_removes_mapping);
    RUN_TEST(test_reannounce_updates_short_addr);
    return UNITY_END();
}
