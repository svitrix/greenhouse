#pragma once
#include <cstdint>

namespace gh::domain {
struct SoilCalibration {
    uint16_t raw_dry;
    uint16_t raw_wet;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return raw_dry < raw_wet;
    }
};
}
