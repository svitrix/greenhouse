#pragma once
#include "ports/IClock.hpp"

namespace gh::test {
class FakeClock : public gh::domain::IClock {
public:
    uint32_t now_ms = 0;
    uint64_t unix_ms = 0;
    void advance(uint32_t delta_ms) noexcept { now_ms += delta_ms; }
    [[nodiscard]] uint32_t nowMs() const noexcept override { return now_ms; }
    [[nodiscard]] uint64_t unixMs() const noexcept override { return unix_ms; }
};
}
