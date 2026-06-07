#pragma once
#include <Preferences.h>
#include "ports/ISoilCalibrationStore.hpp"

namespace gh::infra {
class NvsSoilCalibrationStore final
    : public gh::domain::ISoilCalibrationStore {
public:
    NvsSoilCalibrationStore() noexcept = default;

    [[nodiscard]] gh::domain::ErrorCode begin() noexcept;

    [[nodiscard]] gh::domain::Result<gh::domain::SoilCalibration>
        load() noexcept override;
    [[nodiscard]] gh::domain::ErrorCode
        save(gh::domain::SoilCalibration cal) noexcept override;

private:
    Preferences prefs_;
    bool        opened_ = false;
};
}
