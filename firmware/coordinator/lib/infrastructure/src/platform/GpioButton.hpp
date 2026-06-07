#pragma once
#include "ports/IButton.hpp"

namespace gh::infra {
class GpioButton final : public gh::domain::IButton {
public:
    explicit GpioButton(uint8_t gpio_pin, bool active_low = true) noexcept;
    [[nodiscard]] bool holdConfirmed(uint16_t hold_ms) noexcept override;

private:
    uint8_t gpio_pin_;
    bool    active_low_;
};
}
