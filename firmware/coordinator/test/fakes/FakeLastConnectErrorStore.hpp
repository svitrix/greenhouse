#pragma once
#include "ports/ILastConnectErrorStore.hpp"

namespace gh::test {

class FakeLastConnectErrorStore : public gh::domain::ILastConnectErrorStore {
public:
    gh::domain::ConnectError value = gh::domain::ConnectError::None;
    int                       save_calls = 0;

    [[nodiscard]] gh::domain::ConnectError load() noexcept override {
        return value;
    }

    [[nodiscard]] gh::domain::ErrorCode save(gh::domain::ConnectError err) noexcept override {
        value = err;
        ++save_calls;
        return gh::domain::ErrorCode::Ok;
    }
};

}  // namespace gh::test
