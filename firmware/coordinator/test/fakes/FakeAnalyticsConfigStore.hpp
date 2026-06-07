#pragma once
#include "ports/IAnalyticsConfigStore.hpp"

namespace gh::test {

class FakeAnalyticsConfigStore final : public gh::domain::IAnalyticsConfigStore {
public:
    gh::domain::ErrorCode load(gh::domain::AnalyticsConfig& out) noexcept override {
        if (!present) return gh::domain::ErrorCode::NotFound;
        out = stored;
        return gh::domain::ErrorCode::Ok;
    }
    gh::domain::ErrorCode save(const gh::domain::AnalyticsConfig& in) noexcept override {
        stored = in;
        present = true;
        return gh::domain::ErrorCode::Ok;
    }
    void clear() noexcept override { present = false; }

    gh::domain::AnalyticsConfig stored{};
    bool present = false;
};

}
