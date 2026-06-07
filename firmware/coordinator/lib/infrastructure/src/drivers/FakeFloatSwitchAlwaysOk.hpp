#pragma once
#include "ports/IFloatSwitch.hpp"

namespace gh::infra {
// MVP placeholder: returns true unconditionally.
// Replace with HardwareFloatSwitch (GPIO + INPUT_PULLUP) in iteration 3.
class FakeFloatSwitchAlwaysOk final : public gh::domain::IFloatSwitch {
public:
    [[nodiscard]] bool hasWater() const noexcept override { return true; }
};
}
