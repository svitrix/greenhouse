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
    p.putBool  (kEnabled,  cfg.enabled);
    p.putUChar (kTrigger,  cfg.trigger_below_pct);
    p.putUShort(kInterval, cfg.min_interval_min);
    p.putUChar (kDuration, cfg.duration_s);
    p.putUChar (kMinFresh, cfg.min_fresh_sources);
    p.putULong (kStaleS,   cfg.stale_threshold_s);
    p.end();
    return gh::domain::ErrorCode::Ok;
}

}
