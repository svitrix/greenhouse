#include <unity.h>
#include "registry/InMemoryHistoryStore.hpp"

using gh::domain::NodeId;
using gh::domain::SensorKind;
using gh::infra::InMemoryHistoryStore;
using gh::protocol::Quantity;
using Point = gh::domain::INodeHistoryStore::Point;

void test_record_and_query_returns_points(void) {
    InMemoryHistoryStore h;
    h.recordPoint(NodeId{1}, SensorKind::Air, Quantity::AirTempC,
                  Point{1000, 22.5f});
    h.recordPoint(NodeId{1}, SensorKind::Air, Quantity::AirTempC,
                  Point{2000, 23.0f});

    const auto pts = h.query(NodeId{1}, SensorKind::Air,
                              Quantity::AirTempC, /*since*/ 0);
    TEST_ASSERT_EQUAL_UINT(2, pts.size());
    TEST_ASSERT_EQUAL_FLOAT(22.5f, pts[0].value);
    TEST_ASSERT_EQUAL_FLOAT(23.0f, pts[1].value);
}

void test_query_filters_by_since(void) {
    InMemoryHistoryStore h;
    h.recordPoint(NodeId{1}, SensorKind::Air, Quantity::AirTempC,
                  Point{1000, 22.5f});
    h.recordPoint(NodeId{1}, SensorKind::Air, Quantity::AirTempC,
                  Point{2000, 23.0f});

    const auto pts = h.query(NodeId{1}, SensorKind::Air,
                              Quantity::AirTempC, /*since*/ 1500);
    TEST_ASSERT_EQUAL_UINT(1, pts.size());
    TEST_ASSERT_EQUAL_FLOAT(23.0f, pts[0].value);
}

void test_sliding_24h_window_drops_old(void) {
    InMemoryHistoryStore h;
    constexpr uint32_t kDay = 24u * 60u * 60u * 1000u;
    h.recordPoint(NodeId{1}, SensorKind::Air, Quantity::AirTempC,
                  Point{1000, 1.0f});
    h.recordPoint(NodeId{1}, SensorKind::Air, Quantity::AirTempC,
                  Point{1000 + kDay + 1, 2.0f});

    const auto pts = h.query(NodeId{1}, SensorKind::Air,
                              Quantity::AirTempC, /*since*/ 0);
    TEST_ASSERT_EQUAL_UINT(1, pts.size());
    TEST_ASSERT_EQUAL_FLOAT(2.0f, pts[0].value);
}

void test_forget_node_clears_all_series(void) {
    InMemoryHistoryStore h;
    h.recordPoint(NodeId{1}, SensorKind::Air, Quantity::AirTempC,
                  Point{1000, 1.0f});
    h.recordPoint(NodeId{1}, SensorKind::Soil, Quantity::SoilMoisturePct,
                  Point{1000, 50.0f});
    h.forgetNode(NodeId{1});

    TEST_ASSERT_EQUAL_UINT(0, h.query(NodeId{1}, SensorKind::Air,
                            Quantity::AirTempC, 0).size());
    TEST_ASSERT_EQUAL_UINT(0, h.query(NodeId{1}, SensorKind::Soil,
                            Quantity::SoilMoisturePct, 0).size());
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_record_and_query_returns_points);
    RUN_TEST(test_query_filters_by_since);
    RUN_TEST(test_sliding_24h_window_drops_old);
    RUN_TEST(test_forget_node_clears_all_series);
    return UNITY_END();
}
