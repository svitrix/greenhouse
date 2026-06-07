#pragma once
#include "entities/AutoWaterConfig.hpp"
#include "errors/ErrorCode.hpp"
#include "util/Result.hpp"

namespace gh::domain {
struct IAutoWaterConfigStore {
    virtual ~IAutoWaterConfigStore() = default;
    [[nodiscard]] virtual Result<AutoWaterConfig> load() noexcept = 0;
    [[nodiscard]] virtual ErrorCode save(AutoWaterConfig cfg) noexcept = 0;
};
}
