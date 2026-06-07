#pragma once
#include <cstdint>
#include "ports/IDelay.hpp"

namespace gh::test {

// No-op delay so StatusBlinker::emit() returns instantly under test; records
// the call count and total requested wait so timing intent can be asserted.
struct FakeDelay final : gh::domain::IDelay {
    uint32_t total_ms = 0;
    int      calls = 0;

    void delayMs(uint16_t ms) noexcept override {
        total_ms += ms;
        ++calls;
    }
};

}
