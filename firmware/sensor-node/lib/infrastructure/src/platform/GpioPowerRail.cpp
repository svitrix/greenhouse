#include "GpioPowerRail.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include <Arduino.h>
#include <driver/gpio.h>
#pragma GCC diagnostic pop

namespace gh::infra {

GpioPowerRail::GpioPowerRail(uint8_t gate_pin) noexcept
    : gate_pin_(gate_pin) {}

gh::domain::ErrorCode GpioPowerRail::init() noexcept {
    pinMode(gate_pin_, OUTPUT);
    digitalWrite(gate_pin_, HIGH);
    gpio_hold_dis(static_cast<gpio_num_t>(gate_pin_));
    initialised_ = true;
    is_on_       = false;
    return gh::domain::ErrorCode::Ok;
}

void GpioPowerRail::on() noexcept {
    if (!initialised_) return;
    gpio_hold_dis(static_cast<gpio_num_t>(gate_pin_));
    digitalWrite(gate_pin_, LOW);
    is_on_ = true;
    // NOTE: no implicit delay. Caller waits max(warmupMs) over channels.
}

void GpioPowerRail::off() noexcept {
    if (!initialised_) return;
    digitalWrite(gate_pin_, HIGH);
    gpio_hold_en(static_cast<gpio_num_t>(gate_pin_));
    is_on_ = false;
}

}
