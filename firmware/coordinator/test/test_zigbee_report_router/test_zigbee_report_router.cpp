#include <unity.h>
#include "zigbee/ZigbeeReportRouter.hpp"
#include "registry/InMemoryNodeRegistry.hpp"
#include "registry/InMemoryHistoryStore.hpp"
#include "network/ZigbeeBindingTable.hpp"
#include "fakes/FakeLogger.hpp"

using gh::app::ZigbeeReportRouter;
using gh::domain::ChannelSample;
using gh::domain::NodeId;
using gh::domain::SensorKind;
using gh::infra::InMemoryHistoryStore;
using gh::infra::InMemoryNodeRegistry;
using gh::infra::ZigbeeBindingTable;
using gh::protocol::Quantity;

void test_announce_then_presence_propagates_mask(void) {
    InMemoryNodeRegistry reg;
    InMemoryHistoryStore hist;
    ZigbeeBindingTable   bind;
    gh::test::FakeLogger log;
    ZigbeeReportRouter   router{reg, hist, bind, log};

    router.onDeviceAnnounced(0x1234, 0xAAAAAAAAAAAAAAAAull);
    router.onPresenceFrame  (0x1234, /*source_ieee*/ 0xAAAAAAAAAAAAAAAAull,
                             /*mask*/ 0x07, /*proto*/ 1, /*rssi*/ -55);

    auto snap = reg.snapshot(NodeId{0xAAAAAAAAAAAAAAAAull});
    TEST_ASSERT_TRUE(snap.has_value());
    TEST_ASSERT_EQUAL_UINT32(0x07, snap->present_mask);
    TEST_ASSERT_EQUAL_UINT16(1,    snap->proto_version);
    TEST_ASSERT_EQUAL_INT8 (-55,   snap->last_rssi_dbm);
}

void test_sample_before_announce_is_dropped(void) {
    InMemoryNodeRegistry reg;
    InMemoryHistoryStore hist;
    ZigbeeBindingTable   bind;
    gh::test::FakeLogger log;
    ZigbeeReportRouter   router{reg, hist, bind, log};

    router.onChannelSample(0x9999, /*source_ieee*/ 0xAAull, ChannelSample{
        SensorKind::Air, Quantity::AirTempC, 22.0f, 1000}, /*rssi*/ -60);
    TEST_ASSERT_EQUAL_UINT(0, reg.snapshotAll().size());
    TEST_ASSERT_EQUAL_UINT(0, hist.query(NodeId{0xAA}, SensorKind::Air,
                            Quantity::AirTempC, 0).size());
}

void test_sample_after_announce_appends_to_registry_and_history(void) {
    InMemoryNodeRegistry reg;
    InMemoryHistoryStore hist;
    ZigbeeBindingTable   bind;
    gh::test::FakeLogger log;
    ZigbeeReportRouter   router{reg, hist, bind, log};

    router.onDeviceAnnounced(0x1234, 0xAAAAAAAAAAAAAAAAull);
    router.onChannelSample  (0x1234, /*source_ieee*/ 0xAAAAAAAAAAAAAAAAull,
        ChannelSample{
        SensorKind::Air, Quantity::AirTempC, 22.0f, /*monotonic*/ 1000},
        /*rssi*/ -50);

    auto snap = reg.snapshot(NodeId{0xAAAAAAAAAAAAAAAAull});
    TEST_ASSERT_TRUE(snap.has_value());
    TEST_ASSERT_EQUAL_UINT(1, snap->samples.size());
    TEST_ASSERT_EQUAL_FLOAT(22.0f, snap->samples[0].value_si);

    const auto pts = hist.query(NodeId{0xAAAAAAAAAAAAAAAAull},
                                  SensorKind::Air, Quantity::AirTempC, 0);
    TEST_ASSERT_EQUAL_UINT(1, pts.size());
    TEST_ASSERT_EQUAL_FLOAT(22.0f, pts[0].value);
}

void test_leave_marks_offline_not_forget(void) {
    InMemoryNodeRegistry reg;
    InMemoryHistoryStore hist;
    ZigbeeBindingTable   bind;
    gh::test::FakeLogger log;
    ZigbeeReportRouter   router{reg, hist, bind, log};

    router.onDeviceAnnounced(0x1234, 0xBBull);
    router.onPresenceFrame  (0x1234, /*source_ieee*/ 0xBBull, 0x07, 1, -50);
    router.onDeviceLeft     (0x1234);

    auto snap = reg.snapshot(NodeId{0xBBull});
    TEST_ASSERT_TRUE(snap.has_value());
    TEST_ASSERT_FALSE(snap->online);
    TEST_ASSERT_FALSE(bind.resolve(0x1234).has_value());
}

void test_reannounce_with_new_short_addr_keeps_registry_entry(void) {
    InMemoryNodeRegistry reg;
    InMemoryHistoryStore hist;
    ZigbeeBindingTable   bind;
    gh::test::FakeLogger log;
    ZigbeeReportRouter   router{reg, hist, bind, log};

    router.onDeviceAnnounced(0x1234, 0xCCull);
    router.onPresenceFrame  (0x1234, /*source_ieee*/ 0xCCull, 0x01, 1, -50);
    router.onDeviceAnnounced(0x5678, 0xCCull);  // same IEEE, new short_addr
    router.onChannelSample  (0x5678, /*source_ieee*/ 0xCCull, ChannelSample{
        SensorKind::Air, Quantity::AirTempC, 25.0f, 2000}, -45);

    auto snap = reg.snapshot(NodeId{0xCCull});
    TEST_ASSERT_TRUE(snap.has_value());
    TEST_ASSERT_EQUAL_HEX16(0x5678, snap->short_addr);
    TEST_ASSERT_TRUE(snap->online);
}

// a frame whose APS source IEEE does not match the short_addr binding is a
// spoof and must be rejected — it must not poison the registry/history.
void test_spoofed_source_ieee_is_rejected(void) {
    InMemoryNodeRegistry reg;
    InMemoryHistoryStore hist;
    ZigbeeBindingTable   bind;
    gh::test::FakeLogger log;
    ZigbeeReportRouter   router{reg, hist, bind, log};

    router.onDeviceAnnounced(0x1234, 0xAAAAAAAAAAAAAAAAull);
    // Rogue node reuses 0x1234 but its real IEEE differs from the binding.
    router.onChannelSample  (0x1234, /*source_ieee*/ 0xDEADBEEFDEADBEEFull,
        ChannelSample{SensorKind::Air, Quantity::AirTempC, 99.0f, 1000},
        /*rssi*/ -40);

    auto snap = reg.snapshot(NodeId{0xAAAAAAAAAAAAAAAAull});
    TEST_ASSERT_TRUE(snap.has_value());
    TEST_ASSERT_EQUAL_UINT(0, snap->samples.size());
    TEST_ASSERT_EQUAL_UINT(0, hist.query(NodeId{0xAAAAAAAAAAAAAAAAull},
        SensorKind::Air, Quantity::AirTempC, 0).size());
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_announce_then_presence_propagates_mask);
    RUN_TEST(test_sample_before_announce_is_dropped);
    RUN_TEST(test_sample_after_announce_appends_to_registry_and_history);
    RUN_TEST(test_leave_marks_offline_not_forget);
    RUN_TEST(test_reannounce_with_new_short_addr_keeps_registry_entry);
    RUN_TEST(test_spoofed_source_ieee_is_rejected);
    return UNITY_END();
}
