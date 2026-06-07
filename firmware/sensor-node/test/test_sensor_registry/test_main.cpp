#include <unity.h>
#include "SensorRegistry.hpp"
#include "fakes/FakeSensorChannel.hpp"
#include "fakes/FakeLogger.hpp"

using gh::app::SensorRegistry;
using namespace gh::test;

void setUp(void)    {}
void tearDown(void) {}

namespace {
struct NoopRail : public gh::domain::IPowerRail {
    int on_calls = 0, off_calls = 0;
    bool on_ = false;
    void on()  noexcept override { ++on_calls;  on_ = true;  }
    void off() noexcept override { ++off_calls; on_ = false; }
    [[nodiscard]] bool isOn() const noexcept override { return on_; }
};
}

void test_add_and_iterate_preserves_order(void) {
    SensorRegistry reg;
    FakeSensorChannel a, b, c;
    a.id_ = {0}; b.id_ = {1}; c.id_ = {2};
    reg.add(a); reg.add(b); reg.add(c);
    auto chans = reg.channels();
    TEST_ASSERT_EQUAL(3u, chans.size);
    TEST_ASSERT_EQUAL(0u, chans.data[0]->id().value);
    TEST_ASSERT_EQUAL(1u, chans.data[1]->id().value);
    TEST_ASSERT_EQUAL(2u, chans.data[2]->id().value);
}

void test_probe_all_marks_absent_channels(void) {
    SensorRegistry reg;
    NoopRail rail;
    FakeLogger log;
    FakeSensorChannel ok_ch, absent_ch;
    ok_ch.id_ = {0}; ok_ch.next_probe_status = gh::domain::SensorStatus::Ok;
    absent_ch.id_ = {1}; absent_ch.next_probe_status = gh::domain::SensorStatus::Absent;
    reg.add(ok_ch); reg.add(absent_ch);
    const size_t ok_count = reg.probeAll(rail, log);
    TEST_ASSERT_EQUAL(1u, ok_count);
    TEST_ASSERT_EQUAL(gh::domain::SensorStatus::Ok,     ok_ch.status());
    TEST_ASSERT_EQUAL(gh::domain::SensorStatus::Absent, absent_ch.status());
    // Rail toggled once on then off:
    TEST_ASSERT_EQUAL(1, rail.on_calls);
    TEST_ASSERT_EQUAL(1, rail.off_calls);
}

void test_max_warmup_ignores_absent(void) {
    SensorRegistry reg;
    FakeSensorChannel fast, slow, broken;
    fast.id_   = {0}; fast.warmup_ms_   = 100; fast.status_   = gh::domain::SensorStatus::Ok;
    slow.id_   = {1}; slow.warmup_ms_   = 800; slow.status_   = gh::domain::SensorStatus::Ok;
    broken.id_ = {2}; broken.warmup_ms_ = 5000; broken.status_ = gh::domain::SensorStatus::Absent;
    reg.add(fast); reg.add(slow); reg.add(broken);
    TEST_ASSERT_EQUAL_UINT32(800, reg.maxWarmupMs());
}

void test_present_mask_reflects_statuses(void) {
    SensorRegistry reg;
    FakeSensorChannel a, b, c;
    a.id_ = {0}; a.status_ = gh::domain::SensorStatus::Ok;
    b.id_ = {3}; b.status_ = gh::domain::SensorStatus::Faulty;
    c.id_ = {5}; c.status_ = gh::domain::SensorStatus::Ok;
    reg.add(a); reg.add(b); reg.add(c);
    // bits 0 and 5 only
    TEST_ASSERT_EQUAL_UINT32(0b00100001u, reg.presentMask());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_add_and_iterate_preserves_order);
    RUN_TEST(test_probe_all_marks_absent_channels);
    RUN_TEST(test_max_warmup_ignores_absent);
    RUN_TEST(test_present_mask_reflects_statuses);
    return UNITY_END();
}
