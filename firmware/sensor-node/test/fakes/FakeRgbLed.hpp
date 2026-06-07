#pragma once
#include <cstdint>
#include "ports/IRgbLed.hpp"

namespace gh::test {

// Records every setColor() call in order so a test can assert a full blink
// sequence (on-colours interleaved with off, and the final off).
struct FakeRgbLed final : gh::domain::IRgbLed {
    struct Rgb { uint8_t r; uint8_t g; uint8_t b; };
    static constexpr int kCap = 64;
    Rgb writes[kCap] = {};
    int count = 0;

    void setColor(uint8_t r, uint8_t g, uint8_t b) noexcept override {
        if (count < kCap) { writes[count] = Rgb{r, g, b}; }
        ++count;
    }
};

}
