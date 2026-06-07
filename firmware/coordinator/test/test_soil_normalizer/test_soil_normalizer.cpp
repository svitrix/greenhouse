#include <unity.h>
#include "SoilNormalizer.hpp"
#include "entities/SoilSample.hpp"
#include "fakes/FakeSoilCalibrationStore.hpp"

using gh::app::SoilNormalizer;
using gh::domain::SoilSample;
using gh::domain::SoilCalibration;
using gh::domain::ErrorCode;
using gh::test::FakeSoilCalibrationStore;

namespace {
SoilSample makeRaw(uint16_t raw_cap) {
    SoilSample s{};
    s.timestamp_ms      = 1234;
    s.raw_capacitance   = raw_cap;
    s.moisture_pct      = 0;
    s.temperature_c_x10 = 215;
    return s;
}
}

void test_normalize_midpoint_returns_50pct() {
    FakeSoilCalibrationStore store;
    SoilNormalizer n(store, SoilCalibration{300, 900});
    auto out = n.normalize(makeRaw(600));
    TEST_ASSERT_EQUAL_UINT8(50, out.moisture_pct);
}

void test_normalize_at_dry_boundary_returns_0() {
    FakeSoilCalibrationStore store;
    SoilNormalizer n(store, SoilCalibration{300, 900});
    auto out = n.normalize(makeRaw(300));
    TEST_ASSERT_EQUAL_UINT8(0, out.moisture_pct);
}

void test_normalize_at_wet_boundary_returns_100() {
    FakeSoilCalibrationStore store;
    SoilNormalizer n(store, SoilCalibration{300, 900});
    auto out = n.normalize(makeRaw(900));
    TEST_ASSERT_EQUAL_UINT8(100, out.moisture_pct);
}

void test_normalize_below_dry_clips_to_0() {
    FakeSoilCalibrationStore store;
    SoilNormalizer n(store, SoilCalibration{300, 900});
    auto out = n.normalize(makeRaw(200));
    TEST_ASSERT_EQUAL_UINT8(0, out.moisture_pct);
}

void test_normalize_above_wet_clips_to_100() {
    FakeSoilCalibrationStore store;
    SoilNormalizer n(store, SoilCalibration{300, 900});
    auto out = n.normalize(makeRaw(1000));
    TEST_ASSERT_EQUAL_UINT8(100, out.moisture_pct);
}

void test_normalize_preserves_other_fields() {
    FakeSoilCalibrationStore store;
    SoilNormalizer n(store, SoilCalibration{300, 900});
    auto out = n.normalize(makeRaw(600));
    TEST_ASSERT_EQUAL_UINT32(1234, out.timestamp_ms);
    TEST_ASSERT_EQUAL_UINT16(600, out.raw_capacitance);
    TEST_ASSERT_EQUAL_INT16(215, out.temperature_c_x10);
}

void test_setCalibration_rejects_invalid_keeps_old() {
    FakeSoilCalibrationStore store;
    SoilNormalizer n(store, SoilCalibration{300, 900});
    auto err = n.setCalibration(SoilCalibration{700, 500});  // dry > wet
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::SensorOutOfRange),
                      static_cast<int>(err));
    TEST_ASSERT_EQUAL_INT(0, store.save_calls);
    auto cur = n.calibration();
    TEST_ASSERT_EQUAL_UINT16(300, cur.raw_dry);
    TEST_ASSERT_EQUAL_UINT16(900, cur.raw_wet);
}

void test_setCalibration_writes_through_to_store() {
    FakeSoilCalibrationStore store;
    SoilNormalizer n(store, SoilCalibration{300, 900});
    auto err = n.setCalibration(SoilCalibration{350, 800});
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(err));
    TEST_ASSERT_EQUAL_INT(1, store.save_calls);
    TEST_ASSERT_EQUAL_UINT16(350, store.last_saved.raw_dry);
    TEST_ASSERT_EQUAL_UINT16(800, store.last_saved.raw_wet);
    auto cur = n.calibration();
    TEST_ASSERT_EQUAL_UINT16(350, cur.raw_dry);
    TEST_ASSERT_EQUAL_UINT16(800, cur.raw_wet);
}

void test_setCalibration_store_failure_still_updates_in_RAM() {
    FakeSoilCalibrationStore store;
    store.next_save_error = ErrorCode::ConfigStoreFailed;
    SoilNormalizer n(store, SoilCalibration{300, 900});
    auto err = n.setCalibration(SoilCalibration{350, 800});
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::ConfigStoreFailed),
                      static_cast<int>(err));
    auto cur = n.calibration();
    TEST_ASSERT_EQUAL_UINT16(350, cur.raw_dry);
    TEST_ASSERT_EQUAL_UINT16(800, cur.raw_wet);
}

void test_normalize_with_invalid_initial_returns_pct_0() {
    FakeSoilCalibrationStore store;
    // Constructor accepts invalid calibration silently (noexcept).
    SoilNormalizer n(store, SoilCalibration{500, 500});  // dry == wet
    auto out = n.normalize(makeRaw(600));
    TEST_ASSERT_EQUAL_UINT8(0, out.moisture_pct);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_normalize_midpoint_returns_50pct);
    RUN_TEST(test_normalize_at_dry_boundary_returns_0);
    RUN_TEST(test_normalize_at_wet_boundary_returns_100);
    RUN_TEST(test_normalize_below_dry_clips_to_0);
    RUN_TEST(test_normalize_above_wet_clips_to_100);
    RUN_TEST(test_normalize_preserves_other_fields);
    RUN_TEST(test_setCalibration_rejects_invalid_keeps_old);
    RUN_TEST(test_setCalibration_writes_through_to_store);
    RUN_TEST(test_setCalibration_store_failure_still_updates_in_RAM);
    RUN_TEST(test_normalize_with_invalid_initial_returns_pct_0);
    return UNITY_END();
}
