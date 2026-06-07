#include <unity.h>
#include "drivers/RelayPump.hpp"

using gh::domain::ErrorCode;
using gh::domain::PumpState;
using gh::infra::IGpio;
using gh::infra::RelayPump;

class FakeGpio final : public IGpio {
public:
    int  last_value = -1;
    int  last_mode  = -1;
    int  write_calls = 0;
    int  mode_calls  = 0;
    void pinMode(uint8_t /*pin*/, uint8_t mode) noexcept override {
        last_mode = mode;
        mode_calls++;
    }
    void digitalWrite(uint8_t /*pin*/, uint8_t value) noexcept override {
        last_value = value;
        write_calls++;
    }
};

void test_constructor_forces_safe_low(void) {
    FakeGpio gpio;
    RelayPump pump{gpio, /*pin=*/18};
    TEST_ASSERT_EQUAL(0x03 /*Arduino-ESP32 OUTPUT*/, gpio.last_mode);
    TEST_ASSERT_EQUAL(1, gpio.mode_calls);
    TEST_ASSERT_EQUAL(0 /*LOW*/, gpio.last_value);
    TEST_ASSERT_EQUAL(1, gpio.write_calls);
    TEST_ASSERT_EQUAL(static_cast<int>(PumpState::Off), static_cast<int>(pump.state()));
}

void test_turn_on_sets_high(void) {
    FakeGpio gpio;
    RelayPump pump{gpio, 18};
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok), static_cast<int>(pump.turnOn()));
    TEST_ASSERT_EQUAL(1 /*HIGH*/, gpio.last_value);
    TEST_ASSERT_EQUAL(static_cast<int>(PumpState::On), static_cast<int>(pump.state()));
}

void test_turn_off_sets_low(void) {
    FakeGpio gpio;
    RelayPump pump{gpio, 18};
    (void)pump.turnOn();
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok), static_cast<int>(pump.turnOff()));
    TEST_ASSERT_EQUAL(0 /*LOW*/, gpio.last_value);
    TEST_ASSERT_EQUAL(static_cast<int>(PumpState::Off), static_cast<int>(pump.state()));
}

void test_double_on_keeps_state_on(void) {
    FakeGpio gpio;
    RelayPump pump{gpio, 18};
    const int writes_after_ctor = gpio.write_calls;
    (void)pump.turnOn();
    (void)pump.turnOn();
    TEST_ASSERT_EQUAL(static_cast<int>(PumpState::On), static_cast<int>(pump.state()));
    TEST_ASSERT_EQUAL(1 /*HIGH*/, gpio.last_value);
    // Document current behaviour: each turnOn() writes. If you make it
    // idempotent at the driver level, update the expected count here.
    TEST_ASSERT_EQUAL(writes_after_ctor + 2, gpio.write_calls);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_constructor_forces_safe_low);
    RUN_TEST(test_turn_on_sets_high);
    RUN_TEST(test_turn_off_sets_low);
    RUN_TEST(test_double_on_keeps_state_on);
    return UNITY_END();
}
