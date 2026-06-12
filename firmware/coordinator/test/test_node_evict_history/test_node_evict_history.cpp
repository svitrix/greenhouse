// E7: evicting or forgetting a node must also forget its history series,
// so the bounded history map does not leak orphan series.
#include <unity.h>
#include "registry/InMemoryHistoryStore.hpp"
#include "registry/InMemoryNodeRegistry.hpp"

using gh::domain::ChannelSample;
using gh::domain::ErrorCode;
using gh::domain::NodeId;
using gh::domain::SensorKind;
using gh::infra::InMemoryHistoryStore;
using gh::infra::InMemoryNodeRegistry;
using gh::protocol::Quantity;
using Point = gh::domain::INodeHistoryStore::Point;

static size_t seriesCount(InMemoryHistoryStore& h, NodeId id) {
    return h.query(id, SensorKind::Air, Quantity::AirTempC, 0).size();
}

void test_forget_clears_history(void) {
    InMemoryHistoryStore history;
    InMemoryNodeRegistry reg;
    reg.setHistoryStore(&history);

    (void)reg.recordPresence(NodeId{0xA}, 0x10, 0x01, 1);
    history.recordPoint(NodeId{0xA}, SensorKind::Air, Quantity::AirTempC,
                        Point{1000, 22.0f});
    TEST_ASSERT_EQUAL_UINT(1, seriesCount(history, NodeId{0xA}));

    reg.forget(NodeId{0xA});
    TEST_ASSERT_FALSE(reg.snapshot(NodeId{0xA}).has_value());
    TEST_ASSERT_EQUAL_UINT(0, seriesCount(history, NodeId{0xA}));
}

void test_eviction_clears_history_of_evicted_node(void) {
    InMemoryHistoryStore history;
    InMemoryNodeRegistry reg;
    reg.setHistoryStore(&history);

    for (uint64_t i = 1; i <= 8; ++i) {
        (void)reg.recordPresence(NodeId{i}, static_cast<uint16_t>(i), 0x01, 1);
        reg.recordSample(NodeId{i}, ChannelSample{SensorKind::Air,
            Quantity::AirTempC, 1.0f, static_cast<uint32_t>(i * 1000)});
        history.recordPoint(NodeId{i}, SensorKind::Air, Quantity::AirTempC,
                            Point{static_cast<uint32_t>(i * 1000), 1.0f});
    }
    // Node 1 is the least-recently-seen; mark it offline so it is evicted.
    reg.markOffline(NodeId{1});
    TEST_ASSERT_EQUAL_UINT(1, seriesCount(history, NodeId{1}));

    TEST_ASSERT_EQUAL(ErrorCode::Ok,
                      reg.recordPresence(NodeId{9}, 0x99, 0x01, 1));
    TEST_ASSERT_FALSE(reg.snapshot(NodeId{1}).has_value());
    // Orphan history series for the evicted node is gone.
    TEST_ASSERT_EQUAL_UINT(0, seriesCount(history, NodeId{1}));
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_forget_clears_history);
    RUN_TEST(test_eviction_clears_history_of_evicted_node);
    return UNITY_END();
}
