#pragma once
#include "ports/ISoilCalibrationStore.hpp"

namespace gh::test {
class FakeSoilCalibrationStore : public gh::domain::ISoilCalibrationStore {
public:
    gh::domain::Result<gh::domain::SoilCalibration> next_load{
        gh::domain::ErrorCode::ConfigNotFound, {}};
    gh::domain::ErrorCode next_save_error = gh::domain::ErrorCode::Ok;

    int                          load_calls   = 0;
    int                          save_calls   = 0;
    gh::domain::SoilCalibration  last_saved{};

    [[nodiscard]] gh::domain::Result<gh::domain::SoilCalibration>
        load() noexcept override {
        ++load_calls;
        return next_load;
    }

    [[nodiscard]] gh::domain::ErrorCode
        save(gh::domain::SoilCalibration cal) noexcept override {
        ++save_calls;
        last_saved = cal;
        return next_save_error;
    }
};
}
