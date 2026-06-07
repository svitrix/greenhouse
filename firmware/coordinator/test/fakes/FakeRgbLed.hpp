#pragma once
#include <cstdint>
#include "ports/IRgbLed.hpp"

// Test double: records the last colour written and a write counter so tests
// can assert both the visible colour and that writes are coalesced.
struct FakeRgbLed final : gh::domain::IRgbLed {
    uint8_t r = 0, g = 0, b = 0;
    int     writes = 0;

    void setColor(uint8_t rr, uint8_t gg, uint8_t bb) noexcept override {
        r = rr; g = gg; b = bb; ++writes;
    }

    [[nodiscard]] bool isOff() const noexcept { return r == 0 && g == 0 && b == 0; }
};
