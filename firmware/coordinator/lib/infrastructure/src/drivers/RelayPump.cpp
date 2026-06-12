#include "RelayPump.hpp"

#ifdef ARDUINO
#include <Arduino.h>
namespace gh::infra {
void ArduinoGpio::pinMode(uint8_t pin, uint8_t mode) noexcept       { ::pinMode(pin, mode); }
void ArduinoGpio::digitalWrite(uint8_t pin, uint8_t value) noexcept { ::digitalWrite(pin, value); }
}  // namespace gh::infra
#else
namespace gh::infra {
void ArduinoGpio::pinMode(uint8_t, uint8_t) noexcept       {}
void ArduinoGpio::digitalWrite(uint8_t, uint8_t) noexcept  {}
}  // namespace gh::infra
#endif

namespace gh::infra {

namespace {
#ifdef ARDUINO
// On Arduino-ESP32: OUTPUT=0x03, HIGH=0x1, LOW=0x0 (esp32-hal-gpio.h).
// Pull values from the Arduino headers so they stay accurate across SDK
// updates instead of hardcoding magic numbers.
constexpr uint8_t kOutputMode = OUTPUT;
constexpr uint8_t kHigh       = HIGH;
constexpr uint8_t kLow        = LOW;
#else
// Native test env: FakeGpio just records the values verbatim. We pin them
// to the Arduino-ESP32 values explicitly so the test assertions check the
// *correct* mode (OUTPUT=0x03), and a future drift in Arduino constants
// would be caught by the test on a real MCU.
constexpr uint8_t kOutputMode = 0x03;
constexpr uint8_t kHigh       = 0x1;
constexpr uint8_t kLow        = 0x0;
#endif
}  // namespace

RelayPump::RelayPump(IGpio& gpio, uint8_t pin) noexcept
    : gpio_(gpio), pin_(pin), state_(gh::domain::PumpState::Off) {
    gpio_.pinMode(pin_, kOutputMode);
    gpio_.digitalWrite(pin_, kLow);  // SAFE STATE — first thing after construction
}

// Always returns Ok. Max-runtime safety, float-switch interlock, and any
// PumpState::SafetyLocked transitions are the responsibility of the
// caller (IrrigationService) — this driver is a pure GPIO toggle.
gh::domain::ErrorCode RelayPump::turnOn() noexcept {
    gpio_.digitalWrite(pin_, kHigh);
    state_ = gh::domain::PumpState::On;
    return gh::domain::ErrorCode::Ok;
}

gh::domain::ErrorCode RelayPump::turnOff() noexcept {
    gpio_.digitalWrite(pin_, kLow);
    state_ = gh::domain::PumpState::Off;
    return gh::domain::ErrorCode::Ok;
}

// De-energise the relay and latch the fault state. state() now reports
// SafetyLocked until an explicit turnOff() re-arms the pump.
gh::domain::ErrorCode RelayPump::lock() noexcept {
    gpio_.digitalWrite(pin_, kLow);
    state_ = gh::domain::PumpState::SafetyLocked;
    return gh::domain::ErrorCode::Ok;
}

gh::domain::PumpState RelayPump::state() const noexcept { return state_; }

}  // namespace gh::infra
