#include "NvsAutoWaterConfigStore.hpp"

namespace gh::infra {

gh::domain::Result<gh::domain::AutoWaterConfig>
NvsAutoWaterConfigStore::load() noexcept {
    using R = gh::domain::Result<gh::domain::AutoWaterConfig>;
    Preferences p;
    if (!p.begin(kNs, /*readOnly=*/true)) {
        // First boot — namespace does not exist yet; return defaults
        return R::success(gh::domain::kDefaultAutoWaterConfig);
    }
    gh::domain::AutoWaterConfig c{
        .enabled            = p.getBool  (kEnabled,  gh::domain::kDefaultAutoWaterConfig.enabled),
        .trigger_below_pct  = p.getUChar (kTrigger,  gh::domain::kDefaultAutoWaterConfig.trigger_below_pct),
        .min_interval_min   = p.getUShort(kInterval, gh::domain::kDefaultAutoWaterConfig.min_interval_min),
        .duration_s         = p.getUChar (kDuration, gh::domain::kDefaultAutoWaterConfig.duration_s),
        .min_fresh_sources  = p.getUChar (kMinFresh, gh::domain::kDefaultAutoWaterConfig.min_fresh_sources),
        .stale_threshold_s  = p.getULong (kStaleS,   gh::domain::kDefaultAutoWaterConfig.stale_threshold_s),
    };
    p.end();
    if (!c.valid()) {
        return R::success(gh::domain::kDefaultAutoWaterConfig);
    }
    return R::success(c);
}

gh::domain::ErrorCode
NvsAutoWaterConfigStore::save(gh::domain::AutoWaterConfig cfg) noexcept {
    if (!cfg.valid()) return gh::domain::ErrorCode::ValidationFailed;
    Preferences p;
    if (!p.begin(kNs, /*readOnly=*/false)) {
        return gh::domain::ErrorCode::ConfigStoreFailed;
    }
    // Every putXxx returns the byte count written (0 == failure). A partial
    // write would leave the config inconsistent, so treat any zero as a store
    // failure rather than silently persisting half the record.
    bool ok = true;
    ok &= p.putBool  (kEnabled,  cfg.enabled)            != 0;
    ok &= p.putUChar (kTrigger,  cfg.trigger_below_pct)  != 0;
    ok &= p.putUShort(kInterval, cfg.min_interval_min)   != 0;
    ok &= p.putUChar (kDuration, cfg.duration_s)         != 0;
    ok &= p.putUChar (kMinFresh, cfg.min_fresh_sources)  != 0;
    ok &= p.putULong (kStaleS,   cfg.stale_threshold_s)  != 0;
    p.end();
    return ok ? gh::domain::ErrorCode::Ok
              : gh::domain::ErrorCode::ConfigStoreFailed;
}

}
