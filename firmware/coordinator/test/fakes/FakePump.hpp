#pragma once
#include "ports/IPump.hpp"

namespace gh::test {
class FakePump : public gh::domain::IPump {
public:
    gh::domain::PumpState current = gh::domain::PumpState::Off;
    int on_calls = 0;
    int off_calls = 0;
    int lock_calls = 0;

    gh::domain::ErrorCode turnOn() noexcept override {
        ++on_calls;
        current = gh::domain::PumpState::On;
        return gh::domain::ErrorCode::Ok;
    }
    gh::domain::ErrorCode turnOff() noexcept override {
        ++off_calls;
        current = gh::domain::PumpState::Off;
        return gh::domain::ErrorCode::Ok;
    }
    gh::domain::ErrorCode lock() noexcept override {
        ++lock_calls;
        current = gh::domain::PumpState::SafetyLocked;
        return gh::domain::ErrorCode::Ok;
    }
    [[nodiscard]] gh::domain::PumpState state() const noexcept override {
        return current;
    }
};
}
