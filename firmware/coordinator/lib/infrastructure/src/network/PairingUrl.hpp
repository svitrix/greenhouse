#pragma once
#include <cstddef>
#include <cstring>
#include <cstdio>

namespace gh::infra {

// Derive the pairing-claim URL from the analytics ingest URL configured in the
// captive portal. The field carries "<base>/ingest"; we swap the "/ingest"
// suffix for "/api/pairing/claim". If "/ingest" is absent we append the path.
//
// Pure (only libc string ops) so it is host-testable. Returns false if the
// result would not fit in out (out left NUL-terminated-empty on failure).
[[nodiscard]] inline bool buildClaimUrl(const char* backend_url,
                                        char* out, std::size_t out_size) noexcept {
    if (out == nullptr || out_size == 0) {
        return false;
    }
    out[0] = '\0';
    if (backend_url == nullptr) {
        return false;
    }

    constexpr char kIngest[] = "/ingest";
    constexpr char kClaim[]  = "/api/pairing/claim";

    const char* ingest = std::strstr(backend_url, kIngest);
    std::size_t prefix_len = (ingest != nullptr)
                                 ? static_cast<std::size_t>(ingest - backend_url)
                                 : std::strlen(backend_url);

    // prefix + claim-path + NUL must fit.
    if (prefix_len + std::strlen(kClaim) + 1 > out_size) {
        return false;
    }
    std::memcpy(out, backend_url, prefix_len);
    std::memcpy(out + prefix_len, kClaim, std::strlen(kClaim) + 1);
    return true;
}

}
