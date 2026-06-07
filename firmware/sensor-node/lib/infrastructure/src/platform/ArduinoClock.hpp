#pragma once
#include "ports/IClock.hpp"

namespace gh::infra {
class ArduinoClock final : public gh::domain::IClock {
public:
    [[nodiscard]] uint32_t nowMs() const noexcept override;
    [[nodiscard]] uint64_t unixMs() const noexcept override;
};
}
