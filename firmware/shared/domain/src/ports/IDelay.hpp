#pragma once
#include <cstdint>

namespace gh::domain {

// Port for a blocking millisecond wait. Intended ONLY for terminal pre-sleep
// paths (e.g. status-LED blink codes) — never the report hot path, where the
// no-delay()-over-10ms rule applies. Injected so blocking waits can be made a
// no-op in host tests.
struct IDelay {
    virtual ~IDelay() = default;
    virtual void delayMs(uint16_t ms) noexcept = 0;
};

}
