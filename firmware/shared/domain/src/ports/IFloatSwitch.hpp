#pragma once

namespace gh::domain {
struct IFloatSwitch {
    virtual ~IFloatSwitch() = default;
    [[nodiscard]] virtual bool hasWater() const noexcept = 0;
};
}
