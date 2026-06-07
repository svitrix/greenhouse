#include "SoilNormalizer.hpp"

namespace gh::app {

SoilNormalizer::SoilNormalizer(gh::domain::ISoilCalibrationStore& store,
                               gh::domain::SoilCalibration initial) noexcept
    : store_(store), calibration_(initial) {}

gh::domain::SoilSample
SoilNormalizer::normalize(gh::domain::SoilSample raw) const noexcept {
    if (!calibration_.valid()) {
        raw.moisture_pct = 0;
        return raw;
    }
    const int32_t numerator =
        static_cast<int32_t>(raw.raw_capacitance) - calibration_.raw_dry;
    const int32_t denominator =
        static_cast<int32_t>(calibration_.raw_wet) - calibration_.raw_dry;
    int32_t pct = (numerator * 100) / denominator;
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;
    raw.moisture_pct = static_cast<uint8_t>(pct);
    return raw;
}

gh::domain::ErrorCode
SoilNormalizer::setCalibration(gh::domain::SoilCalibration cal) noexcept {
    if (!cal.valid()) return gh::domain::ErrorCode::SensorOutOfRange;
    calibration_ = cal;
    return store_.save(cal);
}

gh::domain::SoilCalibration
SoilNormalizer::calibration() const noexcept {
    return calibration_;
}

}
