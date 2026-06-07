#pragma once
#include <cstdint>
#include "ports/IRgbLed.hpp"

namespace gh::infra {

// On-board WS2812 RGB LED (DevKitM-1: GPIO8) driven via Arduino-ESP32
// rgbLedWrite(). The raw LED is blinding at full scale, so each channel is
// scaled by brightness_pct (0..100) before output. GRB ordering is handled
// inside rgbLedWrite() (RGB_BUILTIN_LED_COLOR_ORDER).
class Ws2812StatusLed final : public gh::domain::IRgbLed {
public:
    Ws2812StatusLed(uint8_t gpio, uint8_t brightness_pct) noexcept;
    void setColor(uint8_t r, uint8_t g, uint8_t b) noexcept override;

private:
    uint8_t gpio_;
    uint8_t brightness_pct_;
};

}
