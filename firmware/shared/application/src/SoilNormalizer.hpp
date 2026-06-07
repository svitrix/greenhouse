#pragma once
#include "entities/SoilCalibration.hpp"
#include "entities/SoilSample.hpp"
#include "errors/ErrorCode.hpp"
#include "ports/ISoilCalibrationStore.hpp"

namespace gh::app {
class SoilNormalizer {
public:
    SoilNormalizer(gh::domain::ISoilCalibrationStore& store,
                   gh::domain::SoilCalibration initial) noexcept;

    [[nodiscard]] gh::domain::SoilSample
        normalize(gh::domain::SoilSample raw) const noexcept;

    // On store failure the in-RAM calibration is still updated; caller
    // should log/alert but normalization continues with the new values.
    [[nodiscard]] gh::domain::ErrorCode
        setCalibration(gh::domain::SoilCalibration cal) noexcept;

    [[nodiscard]] gh::domain::SoilCalibration calibration() const noexcept;

private:
    gh::domain::ISoilCalibrationStore& store_;
    gh::domain::SoilCalibration        calibration_;
};
}
