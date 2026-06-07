#include <unity.h>
#include "ChannelAttrTable.hpp"
#include "ZclIds.hpp"
#include "entities/SensorKind.hpp"

using gh::protocol::findByZclAddress;
using gh::protocol::kChannelAttrTable;
using gh::protocol::kChannelAttrTableSize;
using gh::protocol::Quantity;
using gh::protocol::presentMaskFor;
using gh::domain::SensorKind;

void test_table_has_all_six_known_entries(void) {
    TEST_ASSERT_EQUAL_UINT(6, kChannelAttrTableSize);
}

void test_findByZclAddress_air_temp(void) {
    const auto* e = findByZclAddress(
        /*ep*/ 1,
        gh::protocol::kClusterTemperatureMeasurement,
        gh::protocol::kAttrMeasuredValue);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(SensorKind::Air,         e->kind);
    TEST_ASSERT_EQUAL(Quantity::AirTempC,      e->quantity);
}

void test_findByZclAddress_soil_temp_on_ep2(void) {
    const auto* e = findByZclAddress(
        /*ep*/ 2,
        gh::protocol::kClusterTemperatureMeasurement,
        gh::protocol::kAttrMeasuredValue);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(SensorKind::Soil,        e->kind);
    TEST_ASSERT_EQUAL(Quantity::SoilTempC,     e->quantity);
}

void test_findByZclAddress_unknown_returns_nullptr(void) {
    TEST_ASSERT_NULL(findByZclAddress(7, 0x1234, 0x5678));
}

void test_every_row_round_trips(void) {
    for (size_t i = 0; i < gh::protocol::kChannelAttrTableSize; ++i) {
        const auto& expected = gh::protocol::kChannelAttrTable[i];
        const auto* found = gh::protocol::findByZclAddress(
            expected.endpoint, expected.cluster_id, expected.attribute_id);
        TEST_ASSERT_NOT_NULL(found);
        TEST_ASSERT_EQUAL(expected.kind,         found->kind);
        TEST_ASSERT_EQUAL(expected.quantity,     found->quantity);
        TEST_ASSERT_EQUAL(expected.channel_id,   found->channel_id);
        TEST_ASSERT_EQUAL(expected.scalar,       found->scalar);
    }
}

void test_presentMaskFor_matches_channel_id_ordinals(void) {
    TEST_ASSERT_EQUAL_UINT32(1u << gh::domain::kSensorChannelIdAir,
                              presentMaskFor(SensorKind::Air));
    TEST_ASSERT_EQUAL_UINT32(1u << gh::domain::kSensorChannelIdSoil1,
                              presentMaskFor(SensorKind::Soil));
    TEST_ASSERT_EQUAL_UINT32(1u << gh::domain::kSensorChannelIdBattery,
                              presentMaskFor(SensorKind::Battery));
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_table_has_all_six_known_entries);
    RUN_TEST(test_findByZclAddress_air_temp);
    RUN_TEST(test_findByZclAddress_soil_temp_on_ep2);
    RUN_TEST(test_findByZclAddress_unknown_returns_nullptr);
    RUN_TEST(test_every_row_round_trips);
    RUN_TEST(test_presentMaskFor_matches_channel_id_ordinals);
    return UNITY_END();
}
