#pragma once
#include <cstdint>
#include "ports/IPump.hpp"

namespace gh::infra {

// Thin GPIO abstraction so RelayPump can be tested on host without Arduino.
// Production implementation `ArduinoGpio` is in RelayPump.cpp under #ifdef
// ARDUINO; host tests substitute a FakeGpio.
struct IGpio {
    virtual ~IGpio() = default;
    virtual void pinMode(uint8_t pin, uint8_t mode) noexcept = 0;
    virtual void digitalWrite(uint8_t pin, uint8_t value) noexcept = 0;
};

class ArduinoGpio final : public IGpio {
public:
    void pinMode(uint8_t pin, uint8_t mode) noexcept override;
    void digitalWrite(uint8_t pin, uint8_t value) noexcept override;
};

// Two-state relay driver. Safe-state LOW asserted in constructor BEFORE any
// other code can race the GPIO. Combined with hardware pull-down 4.7 kOhm on
// the IN1 line, this guarantees pump-off at boot and reset windows.
class RelayPump final : public gh::domain::IPump {
public:
    RelayPump(IGpio& gpio, uint8_t pin) noexcept;
    gh::domain::ErrorCode turnOn()  noexcept override;
    gh::domain::ErrorCode turnOff() noexcept override;
    gh::domain::ErrorCode lock()    noexcept override;
    [[nodiscard]] gh::domain::PumpState state() const noexcept override;
private:
    IGpio&  gpio_;
    uint8_t pin_;
    gh::domain::PumpState state_;
};

}  // namespace gh::infra
