#pragma once
#include <cstdint>
#include "errors/ErrorCode.hpp"

namespace gh::domain {

struct AnalyticsConfig {
    char     backend_url[128];   // "" = analytics disabled
    char     api_key[96];        // 64-char hex token + headroom
    uint32_t flush_period_s;
    bool     insecure_tls;
};

struct IAnalyticsConfigStore {
    virtual ~IAnalyticsConfigStore() = default;
    [[nodiscard]] virtual ErrorCode load(AnalyticsConfig& out) noexcept = 0;
    [[nodiscard]] virtual ErrorCode save(const AnalyticsConfig& in) noexcept = 0;
    virtual void clear() noexcept = 0;
};

}
