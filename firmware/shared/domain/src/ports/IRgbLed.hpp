#pragma once
#include <cstdint>

namespace gh::domain {

// Port for a single addressable RGB LED (e.g. the on-board WS2812). Colors are
// full-range 0..255 per channel; perceived-brightness scaling is the adapter's
// concern, not the caller's.
struct IRgbLed {
    virtual ~IRgbLed() = default;
    virtual void setColor(uint8_t r, uint8_t g, uint8_t b) noexcept = 0;
};

}
