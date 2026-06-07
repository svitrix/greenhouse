#pragma once
#include "ports/IAutoWaterConfigStore.hpp"
#include <Preferences.h>

namespace gh::infra {

class NvsAutoWaterConfigStore final : public gh::domain::IAutoWaterConfigStore {
public:
    NvsAutoWaterConfigStore() noexcept = default;
    [[nodiscard]] gh::domain::Result<gh::domain::AutoWaterConfig> load() noexcept override;
    [[nodiscard]] gh::domain::ErrorCode save(gh::domain::AutoWaterConfig cfg) noexcept override;

private:
    static constexpr const char* kNs       = "auto_water";
    static constexpr const char* kEnabled  = "en";
    static constexpr const char* kTrigger  = "trig";
    static constexpr const char* kInterval = "intv";
    static constexpr const char* kDuration = "dur";
    static constexpr const char* kMinFresh = "minfresh";
    static constexpr const char* kStaleS   = "stale_s";
};

}
