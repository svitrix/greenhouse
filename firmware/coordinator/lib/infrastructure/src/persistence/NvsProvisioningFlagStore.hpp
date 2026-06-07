#pragma once
#include "ports/IProvisioningFlagStore.hpp"
#include <Preferences.h>

namespace gh::infra {

class NvsProvisioningFlagStore final : public gh::domain::IProvisioningFlagStore {
public:
    [[nodiscard]] bool                  isForced()        noexcept override;
    [[nodiscard]] gh::domain::ErrorCode setForced(bool v) noexcept override;
private:
    static constexpr const char* kNs  = "system";
    static constexpr const char* kKey = "force_prov";
};

}
