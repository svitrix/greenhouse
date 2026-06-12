#include <unity.h>
#include "registry/InMemoryNodeRegistry.hpp"

using gh::domain::ChannelSample;
using gh::domain::ErrorCode;
using gh::domain::NodeId;
using gh::domain::SensorKind;
using gh::infra::InMemoryNodeRegistry;
using gh::protocol::Quantity;

void test_record_presence_creates_snapshot(void) {
    InMemoryNodeRegistry reg;
    TEST_ASSERT_EQUAL(ErrorCode::Ok,
                      reg.recordPresence(NodeId{0xA}, 0x1234, 0x07, 1));

    auto snap = reg.snapshot(NodeId{0xA});
    TEST_ASSERT_TRUE(snap.has_value());
    TEST_ASSERT_EQUAL_HEX16(0x1234, snap->short_addr);
    TEST_ASSERT_EQUAL_UINT32(0x07,  snap->present_mask);
    TEST_ASSERT_EQUAL_UINT16(1,     snap->proto_version);
    TEST_ASSERT_TRUE(snap->online);
}

void test_record_sample_updates_or_inserts(void) {
    InMemoryNodeRegistry reg;
    (void)reg.recordPresence(NodeId{0xA}, 0x1234, 0x07, 1);

    reg.recordSample(NodeId{0xA}, ChannelSample{SensorKind::Air,
        Quantity::AirTempC, 22.5f, 1000});
    reg.recordSample(NodeId{0xA}, ChannelSample{SensorKind::Air,
        Quantity::AirTempC, 23.0f, 2000});

    auto snap = reg.snapshot(NodeId{0xA});
    TEST_ASSERT_TRUE(snap.has_value());
    TEST_ASSERT_EQUAL_UINT(1, snap->samples.size());
    TEST_ASSERT_EQUAL_FLOAT(23.0f, snap->samples[0].value_si);
    TEST_ASSERT_EQUAL_UINT32(2000, snap->samples[0].monotonic_ms);
}

void test_snapshot_all_iterates_in_registration_order(void) {
    InMemoryNodeRegistry reg;
    (void)reg.recordPresence(NodeId{0xA}, 0x10, 0x01, 1);
    (void)reg.recordPresence(NodeId{0xB}, 0x20, 0x01, 1);

    const auto all = reg.snapshotAll();
    TEST_ASSERT_EQUAL_UINT(2, all.size());
    TEST_ASSERT_EQUAL_UINT64(0xA, all[0].id.ieee);
    TEST_ASSERT_EQUAL_UINT64(0xB, all[1].id.ieee);
}

void test_capacity_cap_with_offline_eviction(void) {
    InMemoryNodeRegistry reg;
    for (uint64_t i = 1; i <= 8; ++i) {
        TEST_ASSERT_EQUAL(ErrorCode::Ok,
                          reg.recordPresence(NodeId{i},
                              static_cast<uint16_t>(i), 0x01, 1));
    }
    reg.markOffline(NodeId{3});
    TEST_ASSERT_EQUAL(ErrorCode::Ok,
                      reg.recordPresence(NodeId{9}, 0x99, 0x01, 1));
    TEST_ASSERT_FALSE(reg.snapshot(NodeId{3}).has_value());
    TEST_ASSERT_TRUE (reg.snapshot(NodeId{9}).has_value());
}

void test_capacity_cap_no_offline_returns_bounded(void) {
    InMemoryNodeRegistry reg;
    for (uint64_t i = 1; i <= 8; ++i) {
        (void)reg.recordPresence(NodeId{i}, static_cast<uint16_t>(i), 0x01, 1);
    }
    TEST_ASSERT_EQUAL(ErrorCode::BoundedStorageExceeded,
                      reg.recordPresence(NodeId{99}, 0x9999, 0x01, 1));
}

void test_forget_removes_node(void) {
    InMemoryNodeRegistry reg;
    (void)reg.recordPresence(NodeId{0xA}, 0x10, 0x01, 1);
    reg.forget(NodeId{0xA});
    TEST_ASSERT_FALSE(reg.snapshot(NodeId{0xA}).has_value());
}

void test_eviction_picks_oldest_offline_lru(void) {
    InMemoryNodeRegistry reg;
    for (uint64_t i = 1; i <= 8; ++i) {
        (void)reg.recordPresence(NodeId{i}, static_cast<uint16_t>(i), 0x01, 1);
    }
    // Two offline nodes; node 5 is the least-recently-seen, node 2 newer.
    reg.recordSample(NodeId{2}, ChannelSample{SensorKind::Air,
        Quantity::AirTempC, 1.0f, 5'000});
    reg.recordSample(NodeId{5}, ChannelSample{SensorKind::Air,
        Quantity::AirTempC, 1.0f, 1'000});
    reg.markOffline(NodeId{2});
    reg.markOffline(NodeId{5});

    TEST_ASSERT_EQUAL(ErrorCode::Ok,
                      reg.recordPresence(NodeId{9}, 0x99, 0x01, 1));
    // Oldest last_seen_ms (node 5) is evicted; node 2 survives.
    TEST_ASSERT_FALSE(reg.snapshot(NodeId{5}).has_value());
    TEST_ASSERT_TRUE (reg.snapshot(NodeId{2}).has_value());
    TEST_ASSERT_TRUE (reg.snapshot(NodeId{9}).has_value());
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_record_presence_creates_snapshot);
    RUN_TEST(test_record_sample_updates_or_inserts);
    RUN_TEST(test_snapshot_all_iterates_in_registration_order);
    RUN_TEST(test_capacity_cap_with_offline_eviction);
    RUN_TEST(test_capacity_cap_no_offline_returns_bounded);
    RUN_TEST(test_forget_removes_node);
    RUN_TEST(test_eviction_picks_oldest_offline_lru);
    return UNITY_END();
}
