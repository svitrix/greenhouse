#pragma once
#include "entities/PumpState.hpp"
#include "errors/ErrorCode.hpp"

namespace gh::domain {
struct IPump {
    virtual ~IPump() = default;
    virtual ErrorCode turnOn()  noexcept = 0;
    virtual ErrorCode turnOff() noexcept = 0;
    [[nodiscard]] virtual PumpState state() const noexcept = 0;
};
}
