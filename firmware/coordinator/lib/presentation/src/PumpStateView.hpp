#pragma once
#include "entities/PumpState.hpp"

namespace gh::presentation {

// Single source of truth for the pump-state wire code ("OFF"/"ON"/"LOCKED").
// Used by RestPumpRoutes and DashboardViewBuilder; was previously duplicated.
// Pure (domain only) so it compiles under the native test env.
[[nodiscard]] inline const char* pumpStateCode(gh::domain::PumpState s) noexcept {
    switch (s) {
        case gh::domain::PumpState::Off:          return "OFF";
        case gh::domain::PumpState::On:           return "ON";
        case gh::domain::PumpState::SafetyLocked: return "LOCKED";
    }
    return "OFF";
}

}  // namespace gh::presentation
