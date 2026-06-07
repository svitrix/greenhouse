#include <unity.h>
#include <ArduinoJson.h>
#include "DashboardViewBuilder.hpp"
#include "fakes/FakeNodeRegistry.hpp"
#include "fakes/InMemoryAliasStore.hpp"

using gh::presentation::DashboardViewBuilder;
using gh::domain::ChannelSample;
using gh::domain::NodeId;
using gh::domain::NodeSnapshot;
using gh::domain::PumpState;
using gh::domain::SensorKind;
using gh::protocol::Quantity;
using gh::app::AutoWaterDecision;
using gh::app::AutoWaterOutcome;
using gh::test::InMemoryAliasStore;
using gh::test::FakeNodeRegistry;

static NodeSnapshot oneNode(uint32_t now) {
    NodeSnapshot s;
    s.id            = NodeId{0x00124B001A2B3C4Dull};
    s.short_addr    = 0x1A2B;
    s.present_mask  = 0x07;
    s.proto_version = 1;
    s.online        = true;
    s.last_seen_ms  = now - 12'000;
    s.last_rssi_dbm = -52;
    s.samples.push_back(ChannelSample{SensorKind::Air,
        Quantity::AirTempC, 23.4f, s.last_seen_ms});
    return s;
}

void test_dashboard_combines_nodes_pump_auto(void) {
    FakeNodeRegistry reg;
    reg.snapshots.push_back(oneNode(100'000));
    InMemoryAliasStore aliases;

    AutoWaterDecision d{};
    d.outcome      = AutoWaterOutcome::Disabled;
    d.monotonic_ms = 99'000;
    // exercise the fresh_sources serialization path (mirrors RestAutoWaterRoutes)
    d.fresh_sources.push_back(NodeId{0x00124B001A2B3C4Dull});

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    DashboardViewBuilder::build(reg, aliases, PumpState::On, d,
                                /*now_ms*/ 100'000, root);

    TEST_ASSERT_EQUAL(100'000, root["ts_ms"].as<uint32_t>());

    JsonArray nodes = root["nodes"];
    TEST_ASSERT_EQUAL_UINT(1, nodes.size());
    TEST_ASSERT_EQUAL_STRING("00124B001A2B3C4D", nodes[0]["ieee"].as<const char*>());

    TEST_ASSERT_EQUAL_STRING("ON", root["pump"]["state"].as<const char*>());
    TEST_ASSERT_EQUAL(0, root["pump"]["remaining_s"].as<int>());

    TEST_ASSERT_EQUAL_STRING("disabled", root["auto"]["last_decision"].as<const char*>());
    TEST_ASSERT_TRUE(root["auto"]["avg_moisture_pct"].isNull());
    TEST_ASSERT_EQUAL_UINT(1, root["auto"]["fresh_sources"].as<JsonArray>().size());
    TEST_ASSERT_EQUAL_STRING("00124B001A2B3C4D",
        root["auto"]["fresh_sources"][0].as<const char*>());
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_dashboard_combines_nodes_pump_auto);
    return UNITY_END();
}
