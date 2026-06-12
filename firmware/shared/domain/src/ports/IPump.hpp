#pragma once
#include "entities/PumpState.hpp"
#include "errors/ErrorCode.hpp"

namespace gh::domain {
struct IPump {
    virtual ~IPump() = default;
    virtual ErrorCode turnOn()  noexcept = 0;
    virtual ErrorCode turnOff() noexcept = 0;

    // Drive the relay to its safe (off) state and latch PumpState::SafetyLocked
    // so state() reports the fault to every consumer (LED / REST / MQTT) until
    // an explicit turnOff() clears it. Default routes through turnOff() so
    // existing adapters that do not model a latch still de-energise the relay.
    virtual ErrorCode lock() noexcept { return turnOff(); }

    [[nodiscard]] virtual PumpState state() const noexcept = 0;
};
}
