#pragma once
#include "ports/IAutoWaterConfigStore.hpp"

namespace gh::test {

class FakeAutoWaterConfigStore final : public gh::domain::IAutoWaterConfigStore {
public:
    gh::domain::AutoWaterConfig stored{gh::domain::kDefaultAutoWaterConfig};
    int load_calls = 0;
    int save_calls = 0;
    gh::domain::ErrorCode next_load_error = gh::domain::ErrorCode::Ok;
    gh::domain::ErrorCode next_save_error = gh::domain::ErrorCode::Ok;

    gh::domain::Result<gh::domain::AutoWaterConfig> load() noexcept override {
        ++load_calls;
        if (next_load_error != gh::domain::ErrorCode::Ok) {
            return {next_load_error, {}};
        }
        return {gh::domain::ErrorCode::Ok, stored};
    }
    gh::domain::ErrorCode save(gh::domain::AutoWaterConfig cfg) noexcept override {
        ++save_calls;
        if (next_save_error != gh::domain::ErrorCode::Ok) return next_save_error;
        stored = cfg; return gh::domain::ErrorCode::Ok;
    }
};

}
