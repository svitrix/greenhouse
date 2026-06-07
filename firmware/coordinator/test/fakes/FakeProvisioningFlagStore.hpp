#pragma once
#include "ports/IProvisioningFlagStore.hpp"

namespace gh::test {

class FakeProvisioningFlagStore final : public gh::domain::IProvisioningFlagStore {
public:
    bool flag = false;
    int  set_calls = 0;
    gh::domain::ErrorCode next_set_error = gh::domain::ErrorCode::Ok;

    bool isForced() noexcept override { return flag; }

    gh::domain::ErrorCode setForced(bool v) noexcept override {
        ++set_calls;
        if (next_set_error != gh::domain::ErrorCode::Ok) return next_set_error;
        flag = v;
        return gh::domain::ErrorCode::Ok;
    }
};

}
