#pragma once
#include "ports/ISensorCache.hpp"

namespace gh::test {

// Test double for ISensorCache. Allows tests to preload fresh or stale
// samples without touching atomics or timing. updateAir/updateSoil behave
// identically to preloadAir/preloadSoil so production code paths that write
// through the interface are also covered.
class FakeSensorCache final : public gh::domain::ISensorCache {
public:
    void preloadAir(const gh::domain::AirSample& s, uint32_t at_ms) noexcept {
        air_     = s;
        air_at_  = at_ms;
        has_air_ = true;
    }
    void preloadSoil(const gh::domain::SoilSample& s, uint32_t at_ms) noexcept {
        soil_     = s;
        soil_at_  = at_ms;
        has_soil_ = true;
    }
    void clearAir()  noexcept { has_air_  = false; }
    void clearSoil() noexcept { has_soil_ = false; }

    gh::domain::Result<gh::domain::AirSample>
    latestAir(uint32_t now_ms, uint32_t max_age_ms) const noexcept override {
        if (!has_air_ || now_ms - air_at_ > max_age_ms) {
            return gh::domain::Result<gh::domain::AirSample>::failure(
                gh::domain::ErrorCode::SensorNotReady);
        }
        return gh::domain::Result<gh::domain::AirSample>::success(air_);
    }

    gh::domain::Result<gh::domain::SoilSample>
    latestSoil(uint32_t now_ms, uint32_t max_age_ms) const noexcept override {
        if (!has_soil_ || now_ms - soil_at_ > max_age_ms) {
            return gh::domain::Result<gh::domain::SoilSample>::failure(
                gh::domain::ErrorCode::SensorNotReady);
        }
        return gh::domain::Result<gh::domain::SoilSample>::success(soil_);
    }

    void updateAir(const gh::domain::AirSample& s, uint32_t now_ms) noexcept override {
        preloadAir(s, now_ms);
    }
    void updateSoil(const gh::domain::SoilSample& s, uint32_t now_ms) noexcept override {
        preloadSoil(s, now_ms);
    }

private:
    gh::domain::AirSample  air_  {};
    gh::domain::SoilSample soil_ {};
    uint32_t air_at_   = 0;
    uint32_t soil_at_  = 0;
    bool     has_air_  = false;
    bool     has_soil_ = false;
};

}
