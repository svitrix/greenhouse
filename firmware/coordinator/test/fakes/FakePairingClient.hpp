#pragma once
#include <cstring>
#include <string>
#include "ports/IPairingClient.hpp"

namespace gh::test {

class FakePairingClient final : public gh::domain::IPairingClient {
public:
    gh::domain::ErrorCode next_result = gh::domain::ErrorCode::Ok;
    std::string next_api_key = "deadbeef" "deadbeef" "deadbeef" "deadbeef"
                               "deadbeef" "deadbeef" "deadbeef" "deadbeef";

    // Last call's arguments, captured for assertion.
    std::string last_backend_url, last_claim_code, last_device_id,
                last_mac, last_fw_version, last_profile_id;

    gh::domain::ErrorCode claim(
        const char* backend_url,
        const char* claim_code,
        const char* device_id,
        const char* mac,
        const char* fw_version,
        const char* profile_id,
        char*       api_key_out,
        size_t      api_key_out_size) noexcept override
    {
        last_backend_url = backend_url; last_claim_code = claim_code;
        last_device_id = device_id; last_mac = mac;
        last_fw_version = fw_version; last_profile_id = profile_id;

        if (next_result == gh::domain::ErrorCode::Ok && api_key_out != nullptr) {
            std::strncpy(api_key_out, next_api_key.c_str(), api_key_out_size - 1);
            api_key_out[api_key_out_size - 1] = '\0';
        }
        return next_result;
    }
};

}
