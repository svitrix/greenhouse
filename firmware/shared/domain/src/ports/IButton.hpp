#pragma once
#include <cstdint>

namespace gh::domain {
struct IButton {
    virtual ~IButton() = default;
    [[nodiscard]] virtual bool holdConfirmed(uint16_t hold_ms) noexcept = 0;
};
}
