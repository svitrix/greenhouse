#include "Ws2812StatusLed.hpp"
// Arduino-ESP32 headers (WString.h) have benign -Wconversion issues we
// cannot fix upstream. Suppress only around the SDK include.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include <Arduino.h>
#pragma GCC diagnostic pop

namespace gh::infra {

Ws2812StatusLed::Ws2812StatusLed(uint8_t gpio, uint8_t brightness_pct) noexcept
    : gpio_(gpio), brightness_pct_(brightness_pct > 100 ? 100 : brightness_pct) {}

void Ws2812StatusLed::setColor(uint8_t r, uint8_t g, uint8_t b) noexcept {
    const auto scale = [this](uint8_t v) -> uint8_t {
        return static_cast<uint8_t>((static_cast<uint16_t>(v) * brightness_pct_) / 100u);
    };
    rgbLedWrite(gpio_, scale(r), scale(g), scale(b));
}

}  // namespace gh::infra
