#include <unity.h>
#include <ArduinoJson.h>
#include "NodeViewBuilder.hpp"
#include "fakes/InMemoryAliasStore.hpp"

using gh::presentation::NodeViewBuilder;
using gh::domain::ChannelSample;
using gh::domain::NodeId;
using gh::domain::NodeSnapshot;
using gh::domain::SensorKind;
using gh::protocol::Quantity;
using gh::test::InMemoryAliasStore;

static NodeSnapshot snapshotWithAir(uint32_t now) {
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
    s.samples.push_back(ChannelSample{SensorKind::Air,
        Quantity::AirHumidityPct, 56.2f, s.last_seen_ms});
    return s;
}

void test_node_view_contains_required_fields(void) {
    InMemoryAliasStore aliases;
    (void)aliases.setAlias(NodeId{0x00124B001A2B3C4Dull}, "Tomatoes");

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    NodeViewBuilder::build(snapshotWithAir(/*now*/ 100'000), aliases,
                           /*now_ms*/ 100'000, root);

    TEST_ASSERT_EQUAL_STRING("00124B001A2B3C4D", root["ieee"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("0x1A2B",           root["short_addr"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Tomatoes",         root["alias"].as<const char*>());
    TEST_ASSERT_TRUE(root["online"].as<bool>());
    TEST_ASSERT_EQUAL(12,  root["last_seen_s"].as<int>());
    TEST_ASSERT_EQUAL(-52, root["rssi_dbm"].as<int>());
    TEST_ASSERT_EQUAL_STRING("0x07", root["present_mask"].as<const char*>());

    JsonArray readings = root["readings"];
    TEST_ASSERT_EQUAL_UINT(2, readings.size());
    TEST_ASSERT_EQUAL_STRING("air",    readings[0]["kind"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("temp_c", readings[0]["quantity"].as<const char*>());
}

void test_node_view_without_alias_is_null(void) {
    InMemoryAliasStore aliases;
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    NodeViewBuilder::build(snapshotWithAir(100'000), aliases, 100'000, root);
    TEST_ASSERT_TRUE(root["alias"].isNull());
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_node_view_contains_required_fields);
    RUN_TEST(test_node_view_without_alias_is_null);
    return UNITY_END();
}
