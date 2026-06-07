// PasswordHasher
//
// Hashing strategy (path chosen at compile time):
//   - On hardware (ARDUINO): uses mbedTLS bundled with ESP-IDF
//     (<mbedtls/sha256.h> from arduino-esp32's vendored mbedTLS 3.x).
//   - On native test env: uses the vendored public-domain SHA-256
//     reference in sha256_ref.h. Reason: Homebrew's mbedtls 4.x drops
//     the standalone <mbedtls/sha256.h> public header (the algorithm is
//     now routed through PSA crypto only), so we cannot link against the
//     host's libmbedcrypto with the same source. The vendored ref is
//     only used as a test oracle - the production firmware always runs
//     mbedTLS.

#include "PasswordHasher.hpp"

#include <cstring>

#ifdef ARDUINO
#include <mbedtls/sha256.h>
#include "esp_random.h"
#else
#include "sha256_ref.h"
#endif

namespace gh::infra {

void hashPassword(const char* password, const uint8_t salt[16], uint8_t out[32]) noexcept {
    const auto pw_len = std::strlen(password);

#ifdef ARDUINO
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, /*is224=*/0);
    mbedtls_sha256_update(&ctx, salt, 16);
    mbedtls_sha256_update(&ctx, reinterpret_cast<const uint8_t*>(password), pw_len);
    mbedtls_sha256_finish(&ctx, out);
    mbedtls_sha256_free(&ctx);
#else
    sha256_ref::Ctx ctx;
    sha256_ref::init(ctx);
    sha256_ref::update(ctx, salt, 16);
    sha256_ref::update(ctx, reinterpret_cast<const uint8_t*>(password), pw_len);
    sha256_ref::finish(ctx, out);
#endif
}

void generateSalt(uint8_t out[16]) noexcept {
#ifdef ARDUINO
    esp_fill_random(out, 16);
#else
    for (int i = 0; i < 16; ++i) {
        out[i] = static_cast<uint8_t>(0xA0 + i);
    }
#endif
}

}  // namespace gh::infra
