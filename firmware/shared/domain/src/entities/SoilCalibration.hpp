#pragma once
#include <cstdint>

namespace gh::domain {

// Bump on any on-flash layout change. First struct byte; checked on load so a
// record written by a different firmware build is rejected, not blitted as
// garbage calibration into SoilNormalizer.
inline constexpr uint8_t kSoilCalibrationSchemaVersion = 1;

struct SoilCalibration {
    uint8_t  schema_version = kSoilCalibrationSchemaVersion;  // MUST be first member
    uint16_t raw_dry;
    uint16_t raw_wet;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return raw_dry < raw_wet;
    }
};
}
