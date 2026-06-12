#pragma once
#include "ports/IProvisioningFlagStore.hpp"
#include <Preferences.h>

namespace gh::infra {

class NvsProvisioningFlagStore final : public gh::domain::IProvisioningFlagStore {
public:
    [[nodiscard]] bool                  isForced()        noexcept override;
    [[nodiscard]] gh::domain::ErrorCode setForced(bool v) noexcept override;
private:
    // Own namespace per the persistence spec (was "system"/"force_prov").
    // Migration: the old value is intentionally NOT carried over — a stale
    // "force provisioning" flag reading false-by-default on upgrade just lets
    // the device proceed to a normal STA connect, which is the safe default.
    static constexpr const char* kNs  = "prov_flag";
    static constexpr const char* kKey = "pending";
};

}
