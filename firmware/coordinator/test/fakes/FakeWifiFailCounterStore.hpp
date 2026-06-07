#pragma once
#include "ports/IWifiFailCounterStore.hpp"

namespace gh::test {

class FakeWifiFailCounterStore : public gh::domain::IWifiFailCounterStore {
public:
    uint8_t value = 0;
    int     increment_calls = 0;
    int     reset_calls     = 0;

    [[nodiscard]] uint8_t load() noexcept override { return value; }

    [[nodiscard]] gh::domain::ErrorCode increment() noexcept override {
        ++value;
        ++increment_calls;
        return gh::domain::ErrorCode::Ok;
    }

    [[nodiscard]] gh::domain::ErrorCode reset() noexcept override {
        value = 0;
        ++reset_calls;
        return gh::domain::ErrorCode::Ok;
    }
};

}  // namespace gh::test
