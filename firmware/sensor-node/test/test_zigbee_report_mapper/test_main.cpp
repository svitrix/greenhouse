#include <unity.h>
#include "ZigbeeReportMapper.hpp"
#include "ChannelMappings.hpp"
#include "ZclIds.hpp"
#include "fakes/FakeZigbeeEndDevice.hpp"

using gh::infra::ZigbeeReportMapper;
using gh::infra::kChannelMappings;
using namespace gh::test;

void setUp(void)    {}
void tearDown(void) {}

namespace {
gh::domain::SensorReading makeAir(int16_t t_x10, uint16_t h_x10) {
    gh::domain::SensorReading r{};
    r.id   = gh::domain::SensorChannelId{gh::domain::kSensorChannelIdAir};
    r.kind = gh::domain::SensorKind::Air;
    r.values.air = {/*ts=*/0, t_x10, h_x10};
    return r;
}
gh::domain::SensorReading makeSoil(uint16_t raw, int16_t t_x10) {
    gh::domain::SensorReading r{};
    r.id   = gh::domain::SensorChannelId{gh::domain::kSensorChannelIdSoil1};
    r.kind = gh::domain::SensorKind::Soil;
    r.values.soil = {/*ts=*/0, raw, /*pct=*/0, t_x10};
    return r;
}
gh::domain::SensorReading makeBattery(uint16_t mv, uint8_t pct) {
    gh::domain::SensorReading r{};
    r.id   = gh::domain::SensorChannelId{gh::domain::kSensorChannelIdBattery};
    r.kind = gh::domain::SensorKind::Battery;
    r.values.battery = {mv, pct};
    return r;
}
}

void test_air_routed_to_ep1_temp_and_humidity(void) {
    FakeZigbeeEndDevice zb;
    ZigbeeReportMapper mapper{zb, kChannelMappings};
    gh::domain::SensorReading readings[] = { makeAir(/*23.4C*/234, /*56.2%*/562) };
    mapper.publish(readings, 1, /*mask=*/0b001, /*tx_timeout_ms=*/100);

    // Air emits 2 attrs (temp on EP1/cluster 0x0402, humidity on EP1/cluster 0x0405)
    // plus the mask on EP1 Basic 0xF001 = 3 total.
    TEST_ASSERT_EQUAL(3u, zb.attr_call_count);
    bool saw_temp = false, saw_hum = false;
    for (size_t i = 0; i < zb.attr_call_count; ++i) {
        const auto& c = zb.attr_calls[i];
        if (c.endpoint == 1 && c.cluster_id == 0x0402 && c.attribute_id == 0x0000) {
            // ZCL temp = AirSample.temperature_c_x10 * 10 -> 2340
            TEST_ASSERT_EQUAL_UINT32(2340, c.value);
            saw_temp = true;
        }
        if (c.endpoint == 1 && c.cluster_id == 0x0405 && c.attribute_id == 0x0000) {
            TEST_ASSERT_EQUAL_UINT32(5620, c.value);
            saw_hum = true;
        }
    }
    TEST_ASSERT_TRUE(saw_temp);
    TEST_ASSERT_TRUE(saw_hum);
}

void test_soil_moisture_on_ep1_temp_on_ep2(void) {
    FakeZigbeeEndDevice zb;
    ZigbeeReportMapper mapper{zb, kChannelMappings};
    gh::domain::SensorReading readings[] = { makeSoil(512, 195) };
    mapper.publish(readings, 1, /*mask=*/0b010, /*tx_timeout_ms=*/100);

    // Soil emits moisture (EP1 kClusterSoilMoisture) + temp (EP2 cluster 0x0402)
    // plus mask on EP1 Basic = 3 total.
    TEST_ASSERT_EQUAL(3u, zb.attr_call_count);
    bool saw_moist = false, saw_temp = false;
    for (size_t i = 0; i < zb.attr_call_count; ++i) {
        const auto& c = zb.attr_calls[i];
        if (c.endpoint == 1 && c.cluster_id == gh::protocol::kClusterSoilMoisture) {
            saw_moist = true;
        }
        if (c.endpoint == 2 && c.cluster_id == 0x0402) saw_temp  = true;
    }
    TEST_ASSERT_TRUE(saw_moist);
    TEST_ASSERT_TRUE(saw_temp);
}

void test_battery_emits_pct_and_voltage_on_ep1(void) {
    FakeZigbeeEndDevice zb;
    ZigbeeReportMapper mapper{zb, kChannelMappings};
    gh::domain::SensorReading readings[] = { makeBattery(/*4000mV*/4000, /*87%*/87) };
    mapper.publish(readings, 1, /*mask=*/0b100, /*tx_timeout_ms=*/100);

    // Battery emits 2 attrs (pct + voltage on EP1 cluster 0x0001) + mask = 3.
    TEST_ASSERT_EQUAL(3u, zb.attr_call_count);
    bool saw_pct = false, saw_volt = false;
    for (size_t i = 0; i < zb.attr_call_count; ++i) {
        const auto& c = zb.attr_calls[i];
        if (c.endpoint == 1 && c.cluster_id == 0x0001 && c.attribute_id == 0x0021) {
            // batteryPctToZcl(87) = 87*2 = 174
            TEST_ASSERT_EQUAL_UINT32(174, c.value);
            saw_pct = true;
        }
        if (c.endpoint == 1 && c.cluster_id == 0x0001 && c.attribute_id == 0x0020) {
            // (4000 + 50) / 100 = 40
            TEST_ASSERT_EQUAL_UINT32(40, c.value);
            saw_volt = true;
        }
    }
    TEST_ASSERT_TRUE(saw_pct);
    TEST_ASSERT_TRUE(saw_volt);
}

void test_mask_emitted_each_publish(void) {
    FakeZigbeeEndDevice zb;
    ZigbeeReportMapper mapper{zb, kChannelMappings};
    gh::domain::SensorReading readings[] = { makeAir(0, 0) };
    mapper.publish(readings, 1, /*mask=*/0xCAFEBABEu, /*tx_timeout_ms=*/100);

    bool found = false;
    for (size_t i = 0; i < zb.attr_call_count; ++i) {
        const auto& c = zb.attr_calls[i];
        if (c.endpoint == 1 && c.cluster_id == 0x0000 && c.attribute_id == 0xF001) {
            TEST_ASSERT_EQUAL_UINT32(0xCAFEBABEu, c.value);
            found = true;
        }
    }
    TEST_ASSERT_TRUE(found);
}

void test_unmapped_channel_id_is_skipped(void) {
    FakeZigbeeEndDevice zb;
    ZigbeeReportMapper mapper{zb, kChannelMappings};
    gh::domain::SensorReading r{};
    r.id = gh::domain::SensorChannelId{31};   // not in mappings
    r.kind = gh::domain::SensorKind::Air;
    mapper.publish(&r, 1, /*mask=*/0, /*tx_timeout_ms=*/100);

    // Only the mask itself is published (no attrs from the unmapped reading).
    TEST_ASSERT_EQUAL(1u, zb.attr_call_count);
    TEST_ASSERT_EQUAL_HEX16(0xF001, zb.attr_calls[0].attribute_id);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_air_routed_to_ep1_temp_and_humidity);
    RUN_TEST(test_soil_moisture_on_ep1_temp_on_ep2);
    RUN_TEST(test_battery_emits_pct_and_voltage_on_ep1);
    RUN_TEST(test_mask_emitted_each_publish);
    RUN_TEST(test_unmapped_channel_id_is_skipped);
    return UNITY_END();
}
