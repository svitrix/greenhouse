#include <unity.h>
#include <initializer_list>
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
    TEST_ASSERT_EQUAL(AutoWaterOutcome::Stopped, d.outcome);
    TEST_ASSERT_EQUAL_STRING("stop", gh::app::outcomeCode(d.outcome));
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

static NodeSnapshot makeMultiSoilNode(uint64_t ieee, uint32_t now_ms,
                                      uint32_t age_ms,
                                      std::initializer_list<float> moistures) {
    NodeSnapshot s;
    s.id           = NodeId{ieee};
    s.online       = true;
    s.last_seen_ms = now_ms - age_ms;
    for (float m : moistures) {
        ChannelSample sample;
        sample.kind         = SensorKind::Soil;
        sample.quantity     = Quantity::SoilMoisturePct;
        sample.value_si     = m;
        sample.monotonic_ms = now_ms - age_ms;
        s.samples.push_back(sample);
    }
    return s;
}

// After a max-runtime cutoff the pump latches SafetyLocked and tick()
// must NOT re-arm it on the next cycle even with dry soil + water in the tank.
void test_max_runtime_latches_and_blocks_rearm(void) {
    gh::test::FakeNodeRegistry reg;
    gh::test::FakeClock clock;          clock.now_ms = 10'000;
    gh::test::FakePump pump;
    gh::test::FakeFloatSwitch fsw;      fsw.has_water = true;
    gh::test::FakeLogger log;

    reg.snapshots.push_back(makeNode(0xA, 20.0f, clock.now_ms, 1'000));

    gh::domain::AutoWaterConfig cfg = gh::domain::kDefaultAutoWaterConfig;
    cfg.enabled = true;
    cfg.trigger_below_pct = 35;
    cfg.min_fresh_sources = 1;
    cfg.stale_threshold_s = 60;
    cfg.min_interval_min  = 0;

    IrrigationService svc{reg, pump, fsw, clock, log, cfg, /*max_run_ms*/20'000};

    TEST_ASSERT_EQUAL(AutoWaterOutcome::Started, svc.tick().outcome);
    TEST_ASSERT_EQUAL(PumpState::On, pump.state());

    // Advance past max-runtime, keep soil dry samples fresh.
    clock.now_ms += 25'000;
    reg.snapshots.clear();
    reg.snapshots.push_back(makeNode(0xA, 20.0f, clock.now_ms, 1'000));

    const AutoWaterDecision cut = svc.tick();
    TEST_ASSERT_EQUAL(AutoWaterOutcome::LockMaxRuntime, cut.outcome);
    TEST_ASSERT_EQUAL(PumpState::SafetyLocked, pump.state());
    TEST_ASSERT_EQUAL(1, pump.lock_calls);

    // Re-arm must be blocked: a follow-up tick keeps the latch, no new turnOn.
    clock.now_ms += 1'000;
    reg.snapshots.clear();
    reg.snapshots.push_back(makeNode(0xA, 20.0f, clock.now_ms, 100));
    const AutoWaterDecision again = svc.tick();
    TEST_ASSERT_EQUAL(AutoWaterOutcome::LockMaxRuntime, again.outcome);
    TEST_ASSERT_EQUAL(PumpState::SafetyLocked, pump.state());
    TEST_ASSERT_EQUAL(1, pump.on_calls);  // still only the initial start

    // requestOn must also be gated while locked.
    TEST_ASSERT_EQUAL(AutoWaterOutcome::LockMaxRuntime, svc.requestOn().outcome);
    TEST_ASSERT_EQUAL(1, pump.on_calls);

    // Explicit requestOff() re-arms (clears the latch).
    TEST_ASSERT_EQUAL(AutoWaterOutcome::Stopped, svc.requestOff().outcome);
    TEST_ASSERT_EQUAL(PumpState::Off, pump.state());
}

// Manually started pump with auto-water DISABLED must still get a
// dry-tank cutoff on the next tick (check runs before the cfg_.enabled gate).
void test_dry_tank_cutoff_while_disabled(void) {
    gh::test::FakeNodeRegistry reg;
    gh::test::FakeClock clock;          clock.now_ms = 10'000;
    gh::test::FakePump pump;
    gh::test::FakeFloatSwitch fsw;      fsw.has_water = true;
    gh::test::FakeLogger log;

    gh::domain::AutoWaterConfig cfg = gh::domain::kDefaultAutoWaterConfig;
    cfg.enabled = false;
    IrrigationService svc{reg, pump, fsw, clock, log, cfg, 20'000};

    TEST_ASSERT_EQUAL(AutoWaterOutcome::Started, svc.requestOn().outcome);
    TEST_ASSERT_EQUAL(PumpState::On, pump.state());

    // Tank goes dry while running; auto-water still disabled.
    fsw.has_water = false;
    clock.now_ms += 1'000;
    const AutoWaterDecision d = svc.tick();
    TEST_ASSERT_EQUAL(AutoWaterOutcome::LockMaxRuntime, d.outcome);
    TEST_ASSERT_EQUAL(PumpState::SafetyLocked, pump.state());
    TEST_ASSERT_EQUAL(1, pump.lock_calls);
}

// A node reporting several soil samples counts as ONE vote, and 8 nodes
// each with multiple samples do not overflow the bounded source vectors.
void test_multi_sample_node_counts_once(void) {
    gh::test::FakeNodeRegistry reg;
    gh::test::FakeClock clock;          clock.now_ms = 10'000;
    gh::test::FakePump pump;
    gh::test::FakeFloatSwitch fsw;      fsw.has_water = true;
    gh::test::FakeLogger log;

    // Single node, 3 soil samples — average of these would be 30, but it must
    // contribute exactly one fresh source.
    reg.snapshots.push_back(
        makeMultiSoilNode(0xA, clock.now_ms, 1'000, {20.0f, 30.0f, 40.0f}));

    gh::domain::AutoWaterConfig cfg = gh::domain::kDefaultAutoWaterConfig;
    cfg.enabled = true;
    cfg.trigger_below_pct = 90;     // above any sample → SkipAboveThreshold path off
    cfg.min_fresh_sources = 2;      // 1 node < 2 → InsufficientSources proves 1 vote
    cfg.stale_threshold_s = 60;

    IrrigationService svc{reg, pump, fsw, clock, log, cfg, 20'000};
    const AutoWaterDecision d = svc.tick();
    TEST_ASSERT_EQUAL(AutoWaterOutcome::LockInsufficientSources, d.outcome);
    TEST_ASSERT_EQUAL_UINT(1, d.fresh_sources.size());
}

void test_eight_multi_sample_nodes_no_overflow(void) {
    gh::test::FakeNodeRegistry reg;
    gh::test::FakeClock clock;          clock.now_ms = 10'000;
    gh::test::FakePump pump;
    gh::test::FakeFloatSwitch fsw;      fsw.has_water = true;
    gh::test::FakeLogger log;

    // 8 nodes (== kMaxRegisteredNodes), each with the max channel samples.
    for (uint64_t i = 0; i < 8; ++i) {
        reg.snapshots.push_back(makeMultiSoilNode(
            0x100 + i, clock.now_ms, 1'000,
            {20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f}));
    }

    gh::domain::AutoWaterConfig cfg = gh::domain::kDefaultAutoWaterConfig;
    cfg.enabled = true;
    cfg.trigger_below_pct = 90;   // average ~23.5 < 90 → would start
    cfg.min_fresh_sources = 1;
    cfg.stale_threshold_s = 60;
    cfg.min_interval_min  = 0;

    IrrigationService svc{reg, pump, fsw, clock, log, cfg, 20'000};
    const AutoWaterDecision d = svc.tick();  // must not ETL-abort
    TEST_ASSERT_EQUAL(AutoWaterOutcome::Started, d.outcome);
    TEST_ASSERT_EQUAL_UINT(8, d.fresh_sources.size());  // one vote per node
    TEST_ASSERT_TRUE(d.fresh_sources.size() <= gh::domain::kMaxRegisteredNodes);
}

// First auto-start works even when nowMs()==0 (no sentinel collision).
void test_first_autostart_at_time_zero(void) {
    gh::test::FakeNodeRegistry reg;
    gh::test::FakeClock clock;          clock.now_ms = 0;
    gh::test::FakePump pump;
    gh::test::FakeFloatSwitch fsw;      fsw.has_water = true;
    gh::test::FakeLogger log;

    reg.snapshots.push_back(makeNode(0xA, 20.0f, clock.now_ms, 0));

    gh::domain::AutoWaterConfig cfg = gh::domain::kDefaultAutoWaterConfig;
    cfg.enabled = true;
    cfg.trigger_below_pct = 35;
    cfg.min_fresh_sources = 1;
    cfg.stale_threshold_s = 60;
    cfg.min_interval_min  = 30;

    IrrigationService svc{reg, pump, fsw, clock, log, cfg, 20'000};
    const AutoWaterDecision d = svc.tick();
    TEST_ASSERT_EQUAL(AutoWaterOutcome::Started, d.outcome);
    TEST_ASSERT_EQUAL(PumpState::On, pump.state());
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
    RUN_TEST(test_max_runtime_latches_and_blocks_rearm);
    RUN_TEST(test_dry_tank_cutoff_while_disabled);
    RUN_TEST(test_multi_sample_node_counts_once);
    RUN_TEST(test_eight_multi_sample_nodes_no_overflow);
    RUN_TEST(test_first_autostart_at_time_zero);
    return UNITY_END();
}
