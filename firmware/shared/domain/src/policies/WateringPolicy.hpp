#pragma once
#include "entities/SoilSample.hpp"

namespace gh::domain {
enum class WateringDecision { NoAction, StartPump, StopPump };

class WateringPolicy {
public:
    // MVP stub: always returns NoAction. Real algorithm in iteration 2.
    [[nodiscard]] WateringDecision decide(const SoilSample&) const noexcept {
        return WateringDecision::NoAction;
    }
};
}
