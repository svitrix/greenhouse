#pragma once
#include "ports/IMqttCredsStore.hpp"

namespace gh::test {
class FakeMqttCredsStore : public gh::domain::IMqttCredsStore {
public:
    gh::domain::Result<gh::domain::MqttCreds> next_load{
        gh::domain::ErrorCode::ConfigNotFound, {}};
    gh::domain::ErrorCode next_save_error = gh::domain::ErrorCode::Ok;
    int  load_calls = 0;
    int  save_calls = 0;
    gh::domain::MqttCreds last_saved{};

    [[nodiscard]] gh::domain::Result<gh::domain::MqttCreds>
        load() noexcept override { ++load_calls; return next_load; }
    [[nodiscard]] gh::domain::ErrorCode
        save(gh::domain::MqttCreds c) noexcept override {
        ++save_calls; last_saved = c; return next_save_error;
    }
};
}
