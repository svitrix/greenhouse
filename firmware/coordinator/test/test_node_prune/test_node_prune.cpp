#include <unity.h>
#include "node/NodePruneService.hpp"
#include "registry/InMemoryNodeRegistry.hpp"
#include "fakes/FakeClock.hpp"

using gh::domain::NodeId;
using gh::infra::InMemoryNodeRegistry;
using gh::app::NodePruneService;
using gh::test::FakeClock;

void test_recent_node_stays_online(void) {
    InMemoryNodeRegistry reg;
    FakeClock clock; clock.now_ms = 1'000'000;
    (void)reg.recordPresence(NodeId{1}, 0x10, 0x01, 1);
    reg.recordSample(NodeId{1}, {gh::domain::SensorKind::Air,
        gh::protocol::Quantity::AirTempC, 22.0f, clock.now_ms - 30'000});

    NodePruneService prune{reg, clock, /*offline_threshold_ms*/ 60'000};
    prune.tick();
    auto snap = reg.snapshot(NodeId{1});
    TEST_ASSERT_TRUE(snap->online);
}

void test_old_node_marked_offline(void) {
    InMemoryNodeRegistry reg;
    FakeClock clock; clock.now_ms = 1'000'000;
    (void)reg.recordPresence(NodeId{1}, 0x10, 0x01, 1);
    reg.recordSample(NodeId{1}, {gh::domain::SensorKind::Air,
        gh::protocol::Quantity::AirTempC, 22.0f, clock.now_ms - 120'000});

    NodePruneService prune{reg, clock, /*offline_threshold_ms*/ 60'000};
    prune.tick();
    auto snap = reg.snapshot(NodeId{1});
    TEST_ASSERT_FALSE(snap->online);
}

// Regression: a node that announced (presence frame) but never reported a
// sample has last_seen_ms == 0. It must still be prunable past the TTL,
// otherwise its registry slot leaks forever.
void test_never_seen_node_marked_offline(void) {
    InMemoryNodeRegistry reg;
    FakeClock clock; clock.now_ms = 1'000'000;
    (void)reg.recordPresence(NodeId{1}, 0x10, 0x01, 1);

    NodePruneService prune{reg, clock, /*offline_threshold_ms*/ 60'000};
    prune.tick();
    auto snap = reg.snapshot(NodeId{1});
    TEST_ASSERT_FALSE(snap->online);
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_recent_node_stays_online);
    RUN_TEST(test_old_node_marked_offline);
    RUN_TEST(test_never_seen_node_marked_offline);
    return UNITY_END();
}
