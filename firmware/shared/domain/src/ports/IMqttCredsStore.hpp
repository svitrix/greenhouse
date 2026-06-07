#pragma once
#include "entities/MqttCreds.hpp"
#include "errors/ErrorCode.hpp"
#include "util/Result.hpp"

namespace gh::domain {
struct IMqttCredsStore {
    virtual ~IMqttCredsStore() = default;
    [[nodiscard]] virtual Result<MqttCreds> load() noexcept = 0;
    [[nodiscard]] virtual ErrorCode save(MqttCreds creds) noexcept = 0;
};
}
