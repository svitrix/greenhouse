#pragma once
#include <cstdint>

namespace gh::domain {

struct IDeepSleep {
    virtual ~IDeepSleep() = default;

    // Does not return: the device enters deep sleep for wake_up_ms,
    // after wakeup it is a fresh boot (setup() runs again).
    [[noreturn]] virtual void sleepFor(uint32_t wake_up_ms) noexcept = 0;
};

}
