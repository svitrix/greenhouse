#pragma once
#include "ports/IDeepSleep.hpp"

namespace gh::infra {

class DeepSleepClock final : public gh::domain::IDeepSleep {
public:
    DeepSleepClock() noexcept = default;
    [[noreturn]] void sleepFor(uint32_t wake_up_ms) noexcept override;
};

}
