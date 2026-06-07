#pragma once
#include <cstdint>

namespace gh::domain {
enum class PumpState : uint8_t { Off, On, SafetyLocked };

constexpr const char* toString(PumpState s) noexcept {
    switch (s) {
        case PumpState::Off:          return "OFF";
        case PumpState::On:           return "ON";
        case PumpState::SafetyLocked: return "LOCKED";
    }
    return "?";
}
}
