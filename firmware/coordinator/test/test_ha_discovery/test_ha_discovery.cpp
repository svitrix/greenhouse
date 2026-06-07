#include <unity.h>
#include "HomeAssistantDiscoveryService.hpp"
#include "fakes/FakeMqttClient.hpp"
#include "fakes/FakeNodeRegistry.hpp"
#include "fakes/InMemoryAliasStore.hpp"

using gh::presentation::HomeAssistantDiscoveryService;
using gh::domain::ChannelSample;
using gh::domain::NodeId;
using gh::domain::NodeSnapshot;
using gh::domain::SensorKind;
using gh::protocol::Quantity;

void test_new_node_publishes_entity_configs(void) {
    gh::test::FakeMqttClient    mqtt; mqtt.connected = true;
    gh::test::FakeNodeRegistry  reg;
    gh::test::InMemoryAliasStore aliases;

    NodeSnapshot s;
    s.id            = NodeId{0xAAAAAAAAAAAAAAAAull};
    s.online        = true;
    s.present_mask  = (1u << 0) | (1u << 1) | (1u << 2);  // Air | Soil1 | Battery
    reg.snapshots.push_back(s);

    HomeAssistantDiscoveryService svc{mqtt, reg, aliases, "d42"};
    svc.reconcile();

    size_t configs = 0;
    for (const auto& m : mqtt.published) {
        if (m.topic.rfind("homeassistant/sensor/gh_node_AAAAAAAAAAAAAAAA_", 0) == 0 &&
            m.topic.find("/config") != std::string::npos) {
            ++configs;
            TEST_ASSERT_TRUE(m.retain);
            TEST_ASSERT_TRUE(m.payload.find("AAAAAAAAAAAAAAAA") != std::string::npos);
        }
    }
    // 2 air quantities + 2 soil quantities + 2 battery quantities = 6 entities
    TEST_ASSERT_EQUAL_UINT(6, configs);
}

void test_present_mask_bit_cleared_unpublishes(void) {
    gh::test::FakeMqttClient    mqtt; mqtt.connected = true;
    gh::test::FakeNodeRegistry  reg;
    gh::test::InMemoryAliasStore aliases;

    NodeSnapshot s;
    s.id           = NodeId{0xBBBBBBBBBBBBBBBBull};
    s.online       = true;
    s.present_mask = (1u << 0) | (1u << 2);   // Air + Battery
    reg.snapshots.push_back(s);

    HomeAssistantDiscoveryService svc{mqtt, reg, aliases, "d42"};
    svc.reconcile();

    reg.snapshots[0].present_mask = (1u << 0);  // drop battery
    mqtt.published.clear();
    svc.reconcile();
    bool saw_unpublish = false;
    for (const auto& m : mqtt.published) {
        if (m.topic.rfind("homeassistant/sensor/gh_node_BBBBBBBBBBBBBBBB_battery_", 0) == 0 &&
            m.payload.empty() && m.retain) {
            saw_unpublish = true;
        }
    }
    TEST_ASSERT_TRUE(saw_unpublish);
}

void test_forget_node_unpublishes_all(void) {
    gh::test::FakeMqttClient    mqtt; mqtt.connected = true;
    gh::test::FakeNodeRegistry  reg;
    gh::test::InMemoryAliasStore aliases;

    NodeSnapshot s;
    s.id           = NodeId{0xCCCCCCCCCCCCCCCCull};
    s.online       = true;
    s.present_mask = (1u << 0);
    reg.snapshots.push_back(s);

    HomeAssistantDiscoveryService svc{mqtt, reg, aliases, "d42"};
    svc.reconcile();

    reg.snapshots.clear();
    mqtt.published.clear();
    svc.reconcile();

    bool saw_empty = false;
    for (const auto& m : mqtt.published) {
        if (m.topic.rfind("homeassistant/sensor/gh_node_CCCCCCCCCCCCCCCC_", 0) == 0 &&
            m.payload.empty() && m.retain) {
            saw_empty = true;
        }
    }
    TEST_ASSERT_TRUE(saw_empty);
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_new_node_publishes_entity_configs);
    RUN_TEST(test_present_mask_bit_cleared_unpublishes);
    RUN_TEST(test_forget_node_unpublishes_all);
    return UNITY_END();
}
