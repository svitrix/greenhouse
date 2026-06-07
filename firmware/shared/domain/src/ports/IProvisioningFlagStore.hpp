#pragma once
#include "errors/ErrorCode.hpp"

namespace gh::domain {

struct IProvisioningFlagStore {
    virtual ~IProvisioningFlagStore() = default;
    [[nodiscard]] virtual bool      isForced()        noexcept = 0;
    [[nodiscard]] virtual ErrorCode setForced(bool v) noexcept = 0;
};

}
