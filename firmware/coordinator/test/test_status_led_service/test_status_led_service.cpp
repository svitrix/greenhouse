#include <unity.h>
#include "status/StatusLedService.hpp"
#include "fakes/FakeRgbLed.hpp"

using gh::app::StatusLedService;
using gh::app::SystemStatus;
using gh::domain::PumpState;

// ---- arbitrate(pump, wifi_connected, mqtt_connected, mqtt_expected) ----
// priority Fault > Watering > Degraded(wifi/mqtt) > Online

void test_arbitrate_safety_lock_is_fault(void) {
    TEST_ASSERT_EQUAL(static_cast<int>(SystemStatus::Fault),
        static_cast<int>(StatusLedService::arbitrate(PumpState::SafetyLocked, true, true, true)));
}

void test_arbitrate_pump_on_is_watering(void) {
    TEST_ASSERT_EQUAL(static_cast<int>(SystemStatus::Watering),
        static_cast<int>(StatusLedService::arbitrate(PumpState::On, true, false, true)));
}

void test_arbitrate_wifi_down_is_degraded(void) {
    // No MQTT configured, but Wi-Fi link down → Degraded, NOT Online.
    TEST_ASSERT_EQUAL(static_cast<int>(SystemStatus::Degraded),
        static_cast<int>(StatusLedService::arbitrate(PumpState::Off, false, false, false)));
}

void test_arbitrate_mqtt_expected_down_is_degraded(void) {
    TEST_ASSERT_EQUAL(static_cast<int>(SystemStatus::Degraded),
        static_cast<int>(StatusLedService::arbitrate(PumpState::Off, true, false, true)));
}

void test_arbitrate_mqtt_not_expected_is_online(void) {
    TEST_ASSERT_EQUAL(static_cast<int>(SystemStatus::Online),
        static_cast<int>(StatusLedService::arbitrate(PumpState::Off, true, false, false)));
}

void test_arbitrate_mqtt_connected_is_online(void) {
    TEST_ASSERT_EQUAL(static_cast<int>(SystemStatus::Online),
        static_cast<int>(StatusLedService::arbitrate(PumpState::Off, true, true, true)));
}

void test_arbitrate_permit_join_open_is_zigbee_pairing(void) {
    TEST_ASSERT_EQUAL(static_cast<int>(SystemStatus::ZigbeePairing),
        static_cast<int>(StatusLedService::arbitrate(PumpState::Off, true, true, true, true)));
}

void test_arbitrate_pairing_defers_to_fault_and_watering(void) {
    TEST_ASSERT_EQUAL(static_cast<int>(SystemStatus::Fault),
        static_cast<int>(StatusLedService::arbitrate(PumpState::SafetyLocked, true, true, true, true)));
    TEST_ASSERT_EQUAL(static_cast<int>(SystemStatus::Watering),
        static_cast<int>(StatusLedService::arbitrate(PumpState::On, true, true, true, true)));
}

void test_zigbee_pairing_blinks_blue(void) {
    FakeRgbLed led;
    StatusLedService svc{led};
    svc.setStatus(SystemStatus::ZigbeePairing);
    svc.tick(0);
    TEST_ASSERT_EQUAL_UINT8(0,   led.r);
    TEST_ASSERT_EQUAL_UINT8(100, led.g);
    TEST_ASSERT_EQUAL_UINT8(255, led.b);
    svc.tick(600);
    TEST_ASSERT_TRUE(led.isOff());
}

// ---- tick(): solid colour ----

void test_solid_status_writes_full_colour(void) {
    FakeRgbLed led;
    StatusLedService svc{led};
    svc.setStatus(SystemStatus::Online);
    svc.tick(0);
    // Service emits full-range colour; brightness scaling is the adapter's job.
    TEST_ASSERT_EQUAL_UINT8(0,   led.r);
    TEST_ASSERT_EQUAL_UINT8(255, led.g);
    TEST_ASSERT_EQUAL_UINT8(0,   led.b);
}

void test_degraded_is_orange(void) {
    FakeRgbLed led;
    StatusLedService svc{led};
    svc.setStatus(SystemStatus::Degraded);
    svc.tick(0);
    TEST_ASSERT_EQUAL_UINT8(255, led.r);
    TEST_ASSERT_EQUAL_UINT8(70,  led.g);
    TEST_ASSERT_EQUAL_UINT8(0,   led.b);
}

// ---- tick(): write coalescing ----

void test_solid_status_writes_once(void) {
    FakeRgbLed led;
    StatusLedService svc{led};
    svc.setStatus(SystemStatus::Online);
    svc.tick(0);
    svc.tick(100);
    svc.tick(5000);
    TEST_ASSERT_EQUAL_INT(1, led.writes);  // solid → single write
}

// ---- tick(): blink toggles on/off across the period ----

void test_fault_blinks(void) {
    // Fault: red, period 300 ms, on for 150 ms.
    FakeRgbLed led;
    StatusLedService svc{led};
    svc.setStatus(SystemStatus::Fault);

    svc.tick(0);    // phase 0 → on (red)
    TEST_ASSERT_FALSE(led.isOff());
    TEST_ASSERT_EQUAL_UINT8(255, led.r);
    const int writes_after_on = led.writes;

    svc.tick(200);  // phase 200 (>=150) → off
    TEST_ASSERT_TRUE(led.isOff());
    TEST_ASSERT_GREATER_THAN_INT(writes_after_on, led.writes);

    svc.tick(300);  // phase 0 again → on
    TEST_ASSERT_FALSE(led.isOff());
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_arbitrate_safety_lock_is_fault);
    RUN_TEST(test_arbitrate_pump_on_is_watering);
    RUN_TEST(test_arbitrate_wifi_down_is_degraded);
    RUN_TEST(test_arbitrate_mqtt_expected_down_is_degraded);
    RUN_TEST(test_arbitrate_mqtt_not_expected_is_online);
    RUN_TEST(test_arbitrate_mqtt_connected_is_online);
    RUN_TEST(test_arbitrate_permit_join_open_is_zigbee_pairing);
    RUN_TEST(test_arbitrate_pairing_defers_to_fault_and_watering);
    RUN_TEST(test_zigbee_pairing_blinks_blue);
    RUN_TEST(test_solid_status_writes_full_colour);
    RUN_TEST(test_degraded_is_orange);
    RUN_TEST(test_solid_status_writes_once);
    RUN_TEST(test_fault_blinks);
    return UNITY_END();
}
