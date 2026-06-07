#include <unity.h>
#include "irrigation/IrrigationService.hpp"
#include "fakes/FakeNodeRegistry.hpp"
#include "fakes/FakeClock.hpp"
#include "fakes/FakePump.hpp"
#include "fakes/FakeFloatSwitch.hpp"
#include "fakes/FakeLogger.hpp"

using gh::app::AutoWaterOutcome;
using gh::app::AutoWaterDecision;
using gh::app::IrrigationService;
using gh::domain::ChannelSample;
using gh::domain::NodeId;
using gh::domain::NodeSnapshot;
using gh::domain::PumpState;
using gh::domain::SensorKind;
using gh::protocol::Quantity;

static NodeSnapshot makeNode(uint64_t ieee, float moist, uint32_t now_ms,
                              uint32_t age_ms) {
    NodeSnapshot s;
    s.id           = NodeId{ieee};
    s.online       = true;
    s.last_seen_ms = now_ms - age_ms;
    ChannelSample sample;
    sample.kind         = SensorKind::Soil;
    sample.quantity     = Quantity::SoilMoisturePct;
    sample.value_si     = moist;
    sample.monotonic_ms = now_ms - age_ms;
    s.samples.push_back(sample);
    return s;
}

void test_averaging_across_fresh_soils(void) {
    gh::test::FakeNodeRegistry reg;
    gh::test::FakeClock clock;          clock.now_ms = 10'000;
    gh::test::FakePump pump;
    gh::test::FakeFloatSwitch fsw;      fsw.has_water = true;
    gh::test::FakeLogger log;

    reg.snapshots.push_back(makeNode(0xA, 30.0f, clock.now_ms, 1'000));
    reg.snapshots.push_back(makeNode(0xB, 50.0f, clock.now_ms, 1'000));

    gh::domain::AutoWaterConfig cfg = gh::domain::kDefaultAutoWaterConfig;
    cfg.enabled = true;
    cfg.trigger_below_pct = 35;
    cfg.min_fresh_sources = 1;
    cfg.stale_threshold_s = 60;

    IrrigationService svc{reg, pump, fsw, clock, log, cfg, /*max_run_ms*/20'000};
    const AutoWaterDecision d = svc.tick();

    TEST_ASSERT_EQUAL(AutoWaterOutcome::SkipAboveThreshold, d.outcome);
    TEST_ASSERT_TRUE(d.avg_moisture_pct.has_value());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 40.0f, *d.avg_moisture_pct);
    TEST_ASSERT_EQUAL_UINT(2, d.fresh_sources.size());
}

void test_zero_fresh_locks(void) {
    gh::test::FakeNodeRegistry reg;
    gh::test::FakeClock clock;          clock.now_ms = 1'000'000;
    gh::test::FakePump pump;
    gh::test::FakeFloatSwitch fsw;      fsw.has_water = true;
    gh::test::FakeLogger log;

    reg.snapshots.push_back(makeNode(0xA, 20.0f, clock.now_ms, 200'000));

    gh::domain::AutoWaterConfig cfg = gh::domain::kDefaultAutoWaterConfig;
    cfg.enabled = true;
    cfg.trigger_below_pct = 35;
    cfg.min_fresh_sources = 1;
    cfg.stale_threshold_s = 60;

    IrrigationService svc{reg, pump, fsw, clock, log, cfg, 20'000};
    const AutoWaterDecision d = svc.tick();
    TEST_ASSERT_EQUAL(AutoWaterOutcome::LockNoFreshSoil, d.outcome);
    TEST_ASSERT_FALSE(d.avg_moisture_pct.has_value());
}

void test_below_min_fresh_locks(void) {
    gh::test::FakeNodeRegistry reg;
    gh::test::FakeClock clock;          clock.now_ms = 10'000;
    gh::test::FakePump pump;
    gh::test::FakeFloatSwitch fsw;      fsw.has_water = true;
    gh::test::FakeLogger log;

    reg.snapshots.push_back(makeNode(0xA, 20.0f, clock.now_ms, 1'000));

    gh::domain::AutoWaterConfig cfg = gh::domain::kDefaultAutoWaterConfig;
    cfg.enabled = true;
    cfg.trigger_below_pct = 35;
    cfg.min_fresh_sources = 2;
    cfg.stale_threshold_s = 60;

    IrrigationService svc{reg, pump, fsw, clock, log, cfg, 20'000};
    const AutoWaterDecision d = svc.tick();
    TEST_ASSERT_EQUAL(AutoWaterOutcome::LockInsufficientSources, d.outcome);
}

void test_below_threshold_starts_pump(void) {
    gh::test::FakeNodeRegistry reg;
    gh::test::FakeClock clock;          clock.now_ms = 10'000;
    gh::test::FakePump pump;
    gh::test::FakeFloatSwitch fsw;      fsw.has_water = true;
    gh::test::FakeLogger log;

    reg.snapshots.push_back(makeNode(0xA, 20.0f, clock.now_ms, 1'000));
    reg.snapshots.push_back(makeNode(0xB, 25.0f, clock.now_ms, 1'000));

    gh::domain::AutoWaterConfig cfg = gh::domain::kDefaultAutoWaterConfig;
    cfg.enabled = true;
    cfg.trigger_below_pct = 35;
    cfg.min_fresh_sources = 1;
    cfg.stale_threshold_s = 60;
    cfg.duration_s        = 10;
    cfg.min_interval_min  = 30;

    IrrigationService svc{reg, pump, fsw, clock, log, cfg, 20'000};
    const AutoWaterDecision d = svc.tick();
    TEST_ASSERT_EQUAL(AutoWaterOutcome::Started, d.outcome);
    TEST_ASSERT_EQUAL(1, pump.on_calls);
    TEST_ASSERT_EQUAL(PumpState::On, pump.state());
}

void test_disabled_returns_disabled(void) {
    gh::test::FakeNodeRegistry reg;
    gh::test::FakeClock clock;
    gh::test::FakePump pump;
    gh::test::FakeFloatSwitch fsw;      fsw.has_water = true;
    gh::test::FakeLogger log;

    gh::domain::AutoWaterConfig cfg = gh::domain::kDefaultAutoWaterConfig;
    cfg.enabled = false;
    IrrigationService svc{reg, pump, fsw, clock, log, cfg, 20'000};
    TEST_ASSERT_EQUAL(AutoWaterOutcome::Disabled, svc.tick().outcome);
}

void test_manual_request_on_starts_pump(void) {
    gh::test::FakeNodeRegistry reg;
    gh::test::FakeClock clock;          clock.now_ms = 10'000;
    gh::test::FakePump pump;
    gh::test::FakeFloatSwitch fsw;      fsw.has_water = true;
    gh::test::FakeLogger log;

    gh::domain::AutoWaterConfig cfg = gh::domain::kDefaultAutoWaterConfig;
    cfg.enabled = false;   // even when auto is off, manual must work
    IrrigationService svc{reg, pump, fsw, clock, log, cfg, 20'000};

    const AutoWaterDecision d = svc.requestOn();
    TEST_ASSERT_EQUAL(AutoWaterOutcome::Started, d.outcome);
    TEST_ASSERT_EQUAL(1, pump.on_calls);
    TEST_ASSERT_EQUAL(gh::domain::PumpState::On, pump.state());
}

void test_manual_request_off_stops_pump(void) {
    gh::test::FakeNodeRegistry reg;
    gh::test::FakeClock clock;          clock.now_ms = 10'000;
    gh::test::FakePump pump;
    gh::test::FakeFloatSwitch fsw;      fsw.has_water = true;
    gh::test::FakeLogger log;

    gh::domain::AutoWaterConfig cfg = gh::domain::kDefaultAutoWaterConfig;
    IrrigationService svc{reg, pump, fsw, clock, log, cfg, 20'000};
    (void)svc.requestOn();
    const AutoWaterDecision d = svc.requestOff();
    TEST_ASSERT_EQUAL(AutoWaterOutcome::Started, d.outcome);
    TEST_ASSERT_EQUAL(1, pump.off_calls);
    TEST_ASSERT_EQUAL(gh::domain::PumpState::Off, pump.state());
}

void test_manual_request_on_locks_when_no_water(void) {
    gh::test::FakeNodeRegistry reg;
    gh::test::FakeClock clock;          clock.now_ms = 10'000;
    gh::test::FakePump pump;
    gh::test::FakeFloatSwitch fsw;      fsw.has_water = false;
    gh::test::FakeLogger log;

    gh::domain::AutoWaterConfig cfg = gh::domain::kDefaultAutoWaterConfig;
    IrrigationService svc{reg, pump, fsw, clock, log, cfg, 20'000};
    const AutoWaterDecision d = svc.requestOn();
    TEST_ASSERT_EQUAL(AutoWaterOutcome::LockFloatSwitch, d.outcome);
    TEST_ASSERT_EQUAL(0, pump.on_calls);
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_averaging_across_fresh_soils);
    RUN_TEST(test_zero_fresh_locks);
    RUN_TEST(test_below_min_fresh_locks);
    RUN_TEST(test_below_threshold_starts_pump);
    RUN_TEST(test_disabled_returns_disabled);
    RUN_TEST(test_manual_request_on_starts_pump);
    RUN_TEST(test_manual_request_off_stops_pump);
    RUN_TEST(test_manual_request_on_locks_when_no_water);
    return UNITY_END();
}
