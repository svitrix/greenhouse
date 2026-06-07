#pragma once
#include "entities/SoilCalibration.hpp"
#include "errors/ErrorCode.hpp"
#include "util/Result.hpp"

namespace gh::domain {
struct ISoilCalibrationStore {
    virtual ~ISoilCalibrationStore() = default;
    [[nodiscard]] virtual Result<SoilCalibration> load() noexcept = 0;
    [[nodiscard]] virtual ErrorCode save(SoilCalibration cal) noexcept = 0;
};
}
