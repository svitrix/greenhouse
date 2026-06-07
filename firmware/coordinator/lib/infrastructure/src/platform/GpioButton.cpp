#include "GpioButton.hpp"
#include <Arduino.h>

namespace gh::infra {

GpioButton::GpioButton(uint8_t gpio_pin, bool active_low) noexcept
    : gpio_pin_(gpio_pin), active_low_(active_low) {
    pinMode(gpio_pin_, active_low_ ? INPUT_PULLUP : INPUT);
}

bool GpioButton::holdConfirmed(uint16_t hold_ms) noexcept {
    const auto pressed = [this]() {
        return active_low_
            ? digitalRead(gpio_pin_) == LOW
            : digitalRead(gpio_pin_) == HIGH;
    };
    if (!pressed()) return false;

    const uint32_t start = millis();
    while ((millis() - start) < hold_ms) {
        if (!pressed()) return false;
        delay(50);
    }
    return true;
}

}
