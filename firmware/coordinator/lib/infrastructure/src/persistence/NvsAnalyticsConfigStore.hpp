#pragma once
#include <Preferences.h>
#include "ports/IAnalyticsConfigStore.hpp"

namespace gh::infra {

// NVS-backed store for analytics backend config.
// Namespace: "analytics" — disjoint from existing wifi/mqtt/zigbee_net.
// Plain-text storage; see root CLAUDE.md §9.5 for the MVP security trade-off.
class NvsAnalyticsConfigStore final : public gh::domain::IAnalyticsConfigStore {
public:
    NvsAnalyticsConfigStore() = default;

    [[nodiscard]] gh::domain::ErrorCode load(gh::domain::AnalyticsConfig& out) noexcept override;
    [[nodiscard]] gh::domain::ErrorCode save(const gh::domain::AnalyticsConfig& in) noexcept override;
    void clear() noexcept override;

private:
    static constexpr const char* kNamespace = "analytics";
    Preferences prefs_;
};

}
