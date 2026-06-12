#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace gh::infra {

// One-time CSRF nonce for the provisioning form. GET / issues a fresh nonce
// (embedded as a hidden field); POST /save must echo it back and it is
// consumed on first use. This defeats a malicious page auto-submitting /save
// to overwrite Wi-Fi / MQTT / admin creds — it cannot read the nonce minted
// for the real form (same-origin / no-CORS-read), so its POST is rejected 403.
//
// Pure (no Arduino) so the issue/verify/consume state machine is host-testable.
// The 32-bit seed is supplied by the caller (millis()+esp_random() on target);
// uniqueness/unpredictability is the caller's responsibility, this class only
// owns the single-use semantics.
class ProvisioningNonce {
public:
    static constexpr std::size_t kHexLen = 8;  // 32-bit → 8 hex chars

    // Mint a fresh nonce from seed, render it as 8 lowercase hex chars into
    // out (>= kHexLen+1 bytes). Replaces any previously-issued-but-unused nonce.
    void issue(uint32_t seed, char* out, std::size_t out_size) noexcept {
        current_   = seed == 0 ? 1U : seed;  // 0 reserved for "none outstanding"
        has_active_ = true;
        if (out != nullptr && out_size > kHexLen) {
            toHex_(current_, out);
        }
    }

    // Verify candidate matches the outstanding nonce AND consume it (single
    // use). Returns true only on an exact match against an active nonce.
    [[nodiscard]] bool verifyAndConsume(const char* candidate) noexcept {
        if (!has_active_ || candidate == nullptr) {
            return false;
        }
        char expected[kHexLen + 1] = {};
        toHex_(current_, expected);
        const bool ok = std::strncmp(candidate, expected, kHexLen + 1) == 0;
        if (ok) {
            has_active_ = false;  // consume on success
            current_    = 0;
        }
        return ok;
    }

    [[nodiscard]] bool hasActive() const noexcept { return has_active_; }

private:
    static void toHex_(uint32_t v, char* out) noexcept {
        static constexpr char kHex[] = "0123456789abcdef";
        for (int i = 0; i < static_cast<int>(kHexLen); ++i) {
            out[kHexLen - 1 - i] = kHex[v & 0xFU];
            v >>= 4;
        }
        out[kHexLen] = '\0';
    }

    uint32_t current_   = 0;
    bool     has_active_ = false;
};

}
