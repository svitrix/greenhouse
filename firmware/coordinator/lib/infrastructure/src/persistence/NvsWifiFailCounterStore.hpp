#pragma once

#include <Preferences.h>
#include "ports/IWifiFailCounterStore.hpp"

namespace gh::infra {

class NvsWifiFailCounterStore : public gh::domain::IWifiFailCounterStore {
public:
    [[nodiscard]] uint8_t              load()      noexcept override;
    [[nodiscard]] gh::domain::ErrorCode increment() noexcept override;
    [[nodiscard]] gh::domain::ErrorCode reset()    noexcept override;

private:
    // Own namespace per the persistence spec (was multiplexed under "wifi").
    // Migration: the old "wifi"/"fail_count" value is intentionally NOT carried
    // over — this is a transient boot-failure budget, so a one-time reset on
    // upgrade is harmless and avoids a fragile cross-namespace reader.
    static constexpr const char* kNs  = "wifi_fail";
    static constexpr const char* kKey = "count";
};

}  // namespace gh::infra
