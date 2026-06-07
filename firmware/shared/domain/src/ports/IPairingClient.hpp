#pragma once
#include <cstddef>
#include "errors/ErrorCode.hpp"

namespace gh::domain {

struct IPairingClient {
    virtual ~IPairingClient() = default;

    // POST /api/pairing/claim with the supplied claim_code and device
    // identity, expecting JSON {"api_key": "...", "device_id": "..."}.
    // On success writes the api_key (lowercase hex, NUL-terminated) into
    // api_key_out and returns Ok. On 410/409/422 returns a specific
    // ErrorCode so the caller can surface the right captive-portal message.
    //
    // api_key_out_size must be at least 64+1 bytes.
    [[nodiscard]] virtual ErrorCode claim(
        const char* backend_url,
        const char* claim_code,
        const char* device_id,
        const char* mac,
        const char* fw_version,
        const char* profile_id,
        char*       api_key_out,
        size_t      api_key_out_size) noexcept = 0;
};

}
