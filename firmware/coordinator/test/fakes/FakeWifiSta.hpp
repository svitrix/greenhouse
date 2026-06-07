#pragma once
#include <vector>
#include "ports/IWifiSta.hpp"

namespace gh::test {
class FakeWifiSta : public gh::domain::IWifiSta {
public:
    std::vector<gh::domain::ErrorCode> next_connect_results;
    int  connect_calls = 0;
    bool connected_   = false;

    [[nodiscard]] gh::domain::ErrorCode
        connect(const gh::domain::WifiCreds&, uint32_t) noexcept override {
        const auto i = static_cast<size_t>(connect_calls);
        ++connect_calls;
        gh::domain::ErrorCode result =
            (i < next_connect_results.size())
                ? next_connect_results[i]
                : (next_connect_results.empty()
                    ? gh::domain::ErrorCode::Unknown
                    : next_connect_results.back());
        connected_ = (result == gh::domain::ErrorCode::Ok);
        return result;
    }
    [[nodiscard]] bool isConnected() const noexcept override { return connected_; }
};
}
