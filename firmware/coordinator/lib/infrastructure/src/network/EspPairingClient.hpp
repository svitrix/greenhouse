#pragma once
#include "ports/IPairingClient.hpp"
#include "EspHttpsClient.hpp"

namespace gh::infra {

// Concrete IPairingClient on top of EspHttpsClient. The pairing client
// itself is stateless; all TLS state lives in the borrowed EspHttpsClient.
class EspPairingClient final : public gh::domain::IPairingClient {
public:
    explicit EspPairingClient(EspHttpsClient& http) noexcept;

    [[nodiscard]] gh::domain::ErrorCode claim(
        const char* backend_url,
        const char* claim_code,
        const char* device_id,
        const char* mac,
        const char* fw_version,
        const char* profile_id,
        char*       api_key_out,
        size_t      api_key_out_size) noexcept override;

private:
    EspHttpsClient& http_;
};

}
