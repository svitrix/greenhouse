#pragma once
#include <cstdint>
#include "ports/IDelay.hpp"

namespace gh::infra {

// IDelay adapter over Arduino delay(). Used only by StatusBlinker on terminal
// pre-sleep paths (join result / rail fault), so the blocking wait is the
// sanctioned exception to the "no delay() > 10 ms" rule — analogous to the
// bounded SensorCycle warmup wait.
class ArduinoBusyWait final : public gh::domain::IDelay {
public:
    void delayMs(uint16_t ms) noexcept override;
};

}
