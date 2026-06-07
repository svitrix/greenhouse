#pragma once
#include "ports/IWifiCredsStore.hpp"

namespace gh::test {
class FakeWifiCredsStore : public gh::domain::IWifiCredsStore {
public:
    gh::domain::Result<gh::domain::WifiCreds> next_load{
        gh::domain::ErrorCode::ConfigNotFound, {}};
    gh::domain::ErrorCode next_save_error = gh::domain::ErrorCode::Ok;
    int  load_calls = 0;
    int  save_calls = 0;
    gh::domain::WifiCreds last_saved{};

    [[nodiscard]] gh::domain::Result<gh::domain::WifiCreds>
        load() noexcept override { ++load_calls; return next_load; }
    [[nodiscard]] gh::domain::ErrorCode
        save(gh::domain::WifiCreds c) noexcept override {
        ++save_calls; last_saved = c; return next_save_error;
    }
};
}
