#include <unity.h>
#include "telemetry/TelemetryPublisher.hpp"
#include "fakes/FakeNodeRegistry.hpp"
#include "fakes/FakeMqttClient.hpp"
#include "fakes/FakeClock.hpp"

using gh::app::TelemetryPublisher;
using gh::domain::ChannelSample;
using gh::domain::NodeId;
using gh::domain::NodeSnapshot;
using gh::domain::SensorKind;
using gh::protocol::Quantity;
using gh::test::FakeMqttClient;
using gh::test::FakeNodeRegistry;
using gh::test::FakeClock;

static NodeSnapshot oneNode(uint64_t ieee, float air_temp, uint32_t now_ms) {
    NodeSnapshot s;
    s.id           = NodeId{ieee};
    s.short_addr   = 0xABCD;
    s.online       = true;
    s.last_seen_ms = now_ms;
    s.present_mask = 0x01;
    s.samples.push_back(ChannelSample{SensorKind::Air,
        Quantity::AirTempC, air_temp, now_ms});
    return s;
}

void test_publishes_per_quantity_topic_and_retained_metadata(void) {
    FakeMqttClient   mqtt; mqtt.connected = true;
    FakeNodeRegistry reg;
    FakeClock        clock; clock.now_ms = 1000;
    reg.snapshots.push_back(oneNode(0x1122334455667788ull, 22.5f, 1000));

    TelemetryPublisher pub{mqtt, reg, clock, "device42"};
    pub.tick();

    bool seen_temp = false, seen_online = false, seen_present = false;
    for (const auto& m : mqtt.published) {
        if (m.topic == "greenhouse/device42/nodes/1122334455667788/temp_c") {
            seen_temp = true;
            TEST_ASSERT_EQUAL_STRING("22.50", m.payload.c_str());
        }
        if (m.topic == "greenhouse/device42/nodes/1122334455667788/online") {
            seen_online = true;
            TEST_ASSERT_TRUE(m.retain);
            TEST_ASSERT_EQUAL_STRING("true", m.payload.c_str());
        }
        if (m.topic == "greenhouse/device42/nodes/1122334455667788/present_mask") {
            seen_present = true;
            TEST_ASSERT_TRUE(m.retain);
        }
    }
    TEST_ASSERT_TRUE(seen_temp);
    TEST_ASSERT_TRUE(seen_online);
    TEST_ASSERT_TRUE(seen_present);
}

void test_idempotent_tick_does_not_resend_unchanged(void) {
    FakeMqttClient   mqtt; mqtt.connected = true;
    FakeNodeRegistry reg;
    FakeClock        clock; clock.now_ms = 1000;
    reg.snapshots.push_back(oneNode(0xAA, 20.0f, 1000));

    TelemetryPublisher pub{mqtt, reg, clock, "d1"};
    pub.tick();
    // Count topics other than rssi_dbm which republishes every tick.
    auto countNonRssi = [&]() {
        size_t n = 0;
        for (const auto& m : mqtt.published) {
            if (m.topic.find("/rssi_dbm") == std::string::npos) ++n;
        }
        return n;
    };
    const auto after_first = countNonRssi();
    pub.tick();
    TEST_ASSERT_EQUAL_UINT(after_first, countNonRssi());
}

void test_online_change_republishes(void) {
    FakeMqttClient   mqtt; mqtt.connected = true;
    FakeNodeRegistry reg;
    FakeClock        clock; clock.now_ms = 1000;
    reg.snapshots.push_back(oneNode(0xAA, 20.0f, 1000));

    TelemetryPublisher pub{mqtt, reg, clock, "d1"};
    pub.tick();
    const auto before = mqtt.published.size();
    reg.snapshots[0].online = false;
    pub.tick();
    TEST_ASSERT_GREATER_THAN_UINT(before, mqtt.published.size());
}

void test_disconnect_is_noop(void) {
    FakeMqttClient   mqtt; mqtt.connected = false;
    FakeNodeRegistry reg;
    FakeClock        clock; clock.now_ms = 1000;
    reg.snapshots.push_back(oneNode(0xAA, 20.0f, 1000));

    TelemetryPublisher pub{mqtt, reg, clock, "d1"};
    pub.tick();
    TEST_ASSERT_EQUAL_UINT(0, mqtt.published.size());
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_publishes_per_quantity_topic_and_retained_metadata);
    RUN_TEST(test_idempotent_tick_does_not_resend_unchanged);
    RUN_TEST(test_online_change_republishes);
    RUN_TEST(test_disconnect_is_noop);
    return UNITY_END();
}
