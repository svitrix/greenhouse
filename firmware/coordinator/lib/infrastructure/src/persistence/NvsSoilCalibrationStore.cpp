#include "NvsSoilCalibrationStore.hpp"

namespace gh::infra {

gh::domain::ErrorCode NvsSoilCalibrationStore::begin() noexcept {
    if (!prefs_.begin("soil_cal", false)) {
        return gh::domain::ErrorCode::ConfigStoreFailed;
    }
    opened_ = true;
    return gh::domain::ErrorCode::Ok;
}

gh::domain::Result<gh::domain::SoilCalibration>
NvsSoilCalibrationStore::load() noexcept {
    using R = gh::domain::Result<gh::domain::SoilCalibration>;
    if (!opened_) return R::failure(gh::domain::ErrorCode::SensorNotReady);

    gh::domain::SoilCalibration out{};
    const size_t got = prefs_.getBytes("v1", &out, sizeof(out));
    // Size mismatch covers both "no record yet" and a legacy record written by
    // an older firmware whose struct lacked schema_version — treated as absent
    // (caller falls back to AppConfig defaults; next save() writes versioned).
    if (got != sizeof(out)) {
        return R::failure(gh::domain::ErrorCode::ConfigNotFound);
    }
    if (out.schema_version != gh::domain::kSoilCalibrationSchemaVersion) {
        return R::failure(gh::domain::ErrorCode::SensorVersionMismatch);
    }
    if (!out.valid()) {
        return R::failure(gh::domain::ErrorCode::SensorOutOfRange);
    }
    return R::success(out);
}

gh::domain::ErrorCode
NvsSoilCalibrationStore::save(gh::domain::SoilCalibration cal) noexcept {
    if (!opened_)      return gh::domain::ErrorCode::SensorNotReady;
    cal.schema_version = gh::domain::kSoilCalibrationSchemaVersion;
    if (!cal.valid())  return gh::domain::ErrorCode::SensorOutOfRange;

    const size_t written = prefs_.putBytes("v1", &cal, sizeof(cal));
    if (written != sizeof(cal)) {
        return gh::domain::ErrorCode::ConfigStoreFailed;
    }
    return gh::domain::ErrorCode::Ok;
}

}
