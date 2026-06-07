#pragma once
#include <cstdint>

namespace gh::domain {

struct AutoWaterConfig {
    bool     enabled;
    uint8_t  trigger_below_pct;   // 5..80
    uint16_t min_interval_min;    // 5..1440
    uint8_t  duration_s;          // 1..20 (hard-clamped by IrrigationService)
    uint8_t  min_fresh_sources;   // 1..8 (Phase B+)
    uint32_t stale_threshold_s;   // 30..3600 (sample older than this = stale)

    [[nodiscard]] constexpr bool valid() const noexcept {
        return trigger_below_pct >= 5  && trigger_below_pct <= 80
            && min_interval_min  >= 5  && min_interval_min  <= 1440
            && duration_s        >= 1  && duration_s        <= 20
            && min_fresh_sources >= 1  && min_fresh_sources <= 8
            && stale_threshold_s >= 30 && stale_threshold_s <= 3600;
    }
};

constexpr AutoWaterConfig kDefaultAutoWaterConfig{
    .enabled            = false,
    .trigger_below_pct  = 30,
    .min_interval_min   = 60,
    .duration_s         = 15,
    .min_fresh_sources  = 1,
    .stale_threshold_s  = 180,
};

}
