#include <unity.h>
#include "SensorCycle.hpp"
#include "SensorRegistry.hpp"
#include "ZigbeeReportMapper.hpp"
#include "ChannelMappings.hpp"
#include "fakes/FakeSensorChannel.hpp"
#include "fakes/FakeZigbeeEndDevice.hpp"
#include "fakes/FakeClock.hpp"
#include "fakes/FakeLogger.hpp"

using gh::sensor::SensorCycle;
using gh::app::SensorRegistry;
using gh::infra::ZigbeeReportMapper;
using gh::infra::kChannelMappings;
using namespace gh::test;

void setUp(void)    {}
void tearDown(void) {}

namespace {
struct NoopSleep : public gh::domain::IDeepSleep {
    [[noreturn]] void sleepFor(uint32_t) noexcept override { while(true){} }
};
struct NoopRail : public gh::domain::IPowerRail {
    int on_calls = 0, off_calls = 0; bool on_ = false;
    void on()  noexcept override { ++on_calls;  on_ = true;  }
    void off() noexcept override { ++off_calls; on_ = false; }
    [[nodiscard]] bool isOn() const noexcept override { return on_; }
};

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

struct Fixture {
    FakeSensorChannel   air, soil, battery;
    FakeZigbeeEndDevice zb;
    NoopSleep           sleep_stub;
    NoopRail            rail;
    FakeClock           clock;
    FakeLogger          log;
    SensorRegistry      registry;
    ZigbeeReportMapper  mapper;
    SensorCycle         cycle;

    Fixture()
        : mapper(zb, kChannelMappings),
          cycle(registry, rail, mapper, zb, sleep_stub, clock, log, /*tx=*/1500) {
        air.id_ = {gh::domain::kSensorChannelIdAir};
        air.kind_ = gh::domain::SensorKind::Air;
        air.status_ = gh::domain::SensorStatus::Ok;
        soil.id_ = {gh::domain::kSensorChannelIdSoil1};
        soil.kind_ = gh::domain::SensorKind::Soil;
        soil.status_ = gh::domain::SensorStatus::Ok;
        battery.id_ = {gh::domain::kSensorChannelIdBattery};
        battery.kind_ = gh::domain::SensorKind::Battery;
        battery.status_ = gh::domain::SensorStatus::Ok;
        registry.add(air); registry.add(soil); registry.add(battery);
    }
};
}

void test_full_cycle_publishes_and_returns_period(void) {
    Fixture f;
    f.air.next_read     = gh::domain::Result<gh::domain::SensorReading>{
        gh::domain::ErrorCode::Ok, makeAir(234, 562)};
    f.soil.next_read    = gh::domain::Result<gh::domain::SensorReading>{
        gh::domain::ErrorCode::Ok, makeSoil(512, 195)};
    f.battery.next_read = gh::domain::Result<gh::domain::SensorReading>{
        gh::domain::ErrorCode::Ok, makeBattery(3987, 87)};
    f.zb.period_s = 300;
    f.clock.now_ms = 1000;

    const uint32_t sleep_ms = f.cycle.runOnce();

    TEST_ASSERT_EQUAL_UINT32(300U * 1000U, sleep_ms);
    TEST_ASSERT_EQUAL(1, f.rail.on_calls);
    TEST_ASSERT_EQUAL(1, f.rail.off_calls);
    // air=2 attrs + soil=2 attrs + battery=2 attrs + mask=1 = 7 reports
    TEST_ASSERT_EQUAL(7u, f.zb.attr_call_count);
}

void test_one_channel_faulty_skips_its_attrs_keeps_others(void) {
    Fixture f;
    f.soil.status_ = gh::domain::SensorStatus::Faulty;   // skipped before read
    f.air.next_read     = gh::domain::Result<gh::domain::SensorReading>{
        gh::domain::ErrorCode::Ok, makeAir(234, 562)};
    f.battery.next_read = gh::domain::Result<gh::domain::SensorReading>{
        gh::domain::ErrorCode::Ok, makeBattery(3987, 87)};
    f.zb.period_s = 120;

    const uint32_t sleep_ms = f.cycle.runOnce();

    TEST_ASSERT_EQUAL_UINT32(120U * 1000U, sleep_ms);
    // air=2 + battery=2 + mask=1 = 5; soil skipped entirely (0 attrs)
    TEST_ASSERT_EQUAL(5u, f.zb.attr_call_count);
    // mask reflects only Ok channels (bits 0 and 2 — air + battery; soil bit 1 dropped)
    bool found_mask = false;
    for (size_t i = 0; i < f.zb.attr_call_count; ++i) {
        if (f.zb.attr_calls[i].attribute_id == 0xF001) {
            TEST_ASSERT_EQUAL_UINT32(0b101u, f.zb.attr_calls[i].value);
            found_mask = true;
        }
    }
    TEST_ASSERT_TRUE(found_mask);
}

void test_read_failure_transitions_to_faulty_and_logs(void) {
    Fixture f;
    f.air.next_read = gh::domain::Result<gh::domain::SensorReading>::failure(
        gh::domain::ErrorCode::I2cTimeout);
    f.soil.next_read    = gh::domain::Result<gh::domain::SensorReading>{
        gh::domain::ErrorCode::Ok, makeSoil(512, 195)};
    f.battery.next_read = gh::domain::Result<gh::domain::SensorReading>{
        gh::domain::ErrorCode::Ok, makeBattery(3987, 87)};
    f.zb.period_s = 60;

    (void)f.cycle.runOnce();

    // FakeSensorChannel::read() transitions to Faulty on failure.
    TEST_ASSERT_EQUAL(gh::domain::SensorStatus::Faulty, f.air.status());
    TEST_ASSERT_EQUAL(1, f.log.warn_count);
    // The cycle continues — soil + battery attrs still publish.
    // soil=2 + battery=2 + mask=1 = 5 (air was Ok when entered loop, but read failed)
    TEST_ASSERT_EQUAL(5u, f.zb.attr_call_count);
}

void test_all_channels_absent_emits_only_mask(void) {
    Fixture f;
    f.air.status_     = gh::domain::SensorStatus::Absent;
    f.soil.status_    = gh::domain::SensorStatus::Absent;
    f.battery.status_ = gh::domain::SensorStatus::Absent;
    f.zb.period_s = 60;

    (void)f.cycle.runOnce();

    // Only the mask attribute is published.
    TEST_ASSERT_EQUAL(1u, f.zb.attr_call_count);
    TEST_ASSERT_EQUAL_HEX16(0xF001, f.zb.attr_calls[0].attribute_id);
    TEST_ASSERT_EQUAL_UINT32(0u, f.zb.attr_calls[0].value);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_full_cycle_publishes_and_returns_period);
    RUN_TEST(test_one_channel_faulty_skips_its_attrs_keeps_others);
    RUN_TEST(test_read_failure_transitions_to_faulty_and_logs);
    RUN_TEST(test_all_channels_absent_emits_only_mask);
    return UNITY_END();
}
