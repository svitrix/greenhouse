// PasswordHasher
//
// KDF strategy (path chosen at compile time):
//   - On hardware (ARDUINO): PBKDF2-HMAC-SHA256 via mbedTLS bundled with
//     ESP-IDF (<mbedtls/pkcs5.h> + <mbedtls/md.h> from arduino-esp32's
//     vendored mbedTLS 3.x). esp_fill_random provides the salt RNG.
//   - On native test env: PBKDF2-HMAC-SHA256 computed on top of the vendored
//     public-domain SHA-256 reference (sha256_ref.h). Reason: Homebrew's
//     mbedtls 4.x drops the standalone <mbedtls/sha256.h> public header (the
//     algorithm is now routed through PSA crypto only), so we cannot link the
//     host against libmbedcrypto with the same source. The vendored ref is a
//     bit-exact PBKDF2 oracle for unit tests; production firmware always runs
//     mbedTLS.
//
// SECURITY INVARIANT: the deterministic host salt fallback that used to
// live in generateSalt() is GONE. generateSalt() is now defined only under
// ARDUINO. A host build has no secure RNG and must supply salt bytes itself.

#include "PasswordHasher.hpp"

#include <cstring>

// GH_REAL_CRYPTO is a tripwire: a build that claims to be "real crypto" (a
// shipped firmware) MUST go through the ARDUINO/mbedTLS path. If it is ever
// set without ARDUINO, the build would otherwise silently use the host
// reference KDF with no hardware RNG for salts - refuse to compile.
#if defined(GH_REAL_CRYPTO) && !defined(ARDUINO)
#error "GH_REAL_CRYPTO requires the ARDUINO/mbedTLS path: no secure RNG (generateSalt) exists off-target."
#endif

#ifdef ARDUINO
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/sha256.h>
#include "esp_random.h"
#else
#include "sha256_ref.h"
#endif

namespace gh::infra {

namespace {

constexpr std::size_t kSaltLen   = 16;
constexpr std::size_t kHashLen   = 32;
constexpr std::size_t kSha256Len = 32;

#ifndef ARDUINO
// --- Host-only PBKDF2-HMAC-SHA256 on top of sha256_ref ---------------------
// Used purely as a test oracle. Bit-exact with mbedtls_pkcs5_pbkdf2_hmac so
// native test vectors carry over to the firmware path.

constexpr std::size_t kBlockLen = 64;  // SHA-256 block size

void sha256(const uint8_t* data, std::size_t len, uint8_t out[kSha256Len]) noexcept {
    sha256_ref::Ctx ctx;
    sha256_ref::init(ctx);
    sha256_ref::update(ctx, data, len);
    sha256_ref::finish(ctx, out);
}

// HMAC-SHA256 over two concatenated message chunks (key derivation only needs
// salt||INT and prev-block as the message, so a 2-chunk variant suffices).
void hmacSha256(const uint8_t* key, std::size_t key_len,
                const uint8_t* msg_a, std::size_t msg_a_len,
                const uint8_t* msg_b, std::size_t msg_b_len,
                uint8_t out[kSha256Len]) noexcept {
    uint8_t k0[kBlockLen] = {};
    if (key_len > kBlockLen) {
        sha256(key, key_len, k0);  // first kSha256Len bytes, rest stays zero
    } else {
        std::memcpy(k0, key, key_len);
    }

    uint8_t ipad[kBlockLen];
    uint8_t opad[kBlockLen];
    for (std::size_t i = 0; i < kBlockLen; ++i) {
        ipad[i] = static_cast<uint8_t>(k0[i] ^ 0x36);
        opad[i] = static_cast<uint8_t>(k0[i] ^ 0x5c);
    }

    uint8_t inner[kSha256Len];
    {
        sha256_ref::Ctx ctx;
        sha256_ref::init(ctx);
        sha256_ref::update(ctx, ipad, kBlockLen);
        if (msg_a_len > 0) sha256_ref::update(ctx, msg_a, msg_a_len);
        if (msg_b_len > 0) sha256_ref::update(ctx, msg_b, msg_b_len);
        sha256_ref::finish(ctx, inner);
    }

    sha256_ref::Ctx ctx;
    sha256_ref::init(ctx);
    sha256_ref::update(ctx, opad, kBlockLen);
    sha256_ref::update(ctx, inner, kSha256Len);
    sha256_ref::finish(ctx, out);
}

// Single-block PBKDF2 (dkLen == hLen == 32, so exactly one output block).
void pbkdf2HmacSha256(const uint8_t* password, std::size_t password_len,
                      const uint8_t* salt, std::size_t salt_len,
                      uint32_t iterations,
                      uint8_t out[kHashLen]) noexcept {
    const uint8_t block_index[4] = {0x00, 0x00, 0x00, 0x01};  // big-endian INT(1)

    uint8_t u[kSha256Len];
    hmacSha256(password, password_len, salt, salt_len, block_index, sizeof(block_index), u);

    uint8_t t[kSha256Len];
    std::memcpy(t, u, kSha256Len);

    for (uint32_t iter = 1; iter < iterations; ++iter) {
        hmacSha256(password, password_len, u, kSha256Len, nullptr, 0, u);
        for (std::size_t i = 0; i < kSha256Len; ++i) {
            t[i] = static_cast<uint8_t>(t[i] ^ u[i]);
        }
    }

    std::memcpy(out, t, kHashLen);
}

void legacySingleSha(const uint8_t* salt, const char* password,
                     std::size_t pw_len, uint8_t out[kHashLen]) noexcept {
    sha256_ref::Ctx ctx;
    sha256_ref::init(ctx);
    sha256_ref::update(ctx, salt, kSaltLen);
    sha256_ref::update(ctx, reinterpret_cast<const uint8_t*>(password), pw_len);
    sha256_ref::finish(ctx, out);
}
#else
void legacySingleSha(const uint8_t* salt, const char* password,
                     std::size_t pw_len, uint8_t out[kHashLen]) noexcept {
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, /*is224=*/0);
    mbedtls_sha256_update(&ctx, salt, kSaltLen);
    mbedtls_sha256_update(&ctx, reinterpret_cast<const uint8_t*>(password), pw_len);
    mbedtls_sha256_finish(&ctx, out);
    mbedtls_sha256_free(&ctx);
}
#endif

}  // namespace

void hashPassword(const char* password,
                  const uint8_t salt[16],
                  uint32_t iterations,
                  uint8_t out[32]) noexcept {
    const std::size_t pw_len = std::strlen(password);

    if (iterations == 0) {  // legacy verification path only
        legacySingleSha(salt, password, pw_len, out);
        return;
    }

#ifdef ARDUINO
    const mbedtls_md_info_t* md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    // hmac=1: the context is set up for HMAC, as pbkdf2 requires.
    if (mbedtls_md_setup(&ctx, md, /*hmac=*/1) == 0) {
        mbedtls_pkcs5_pbkdf2_hmac(
            &ctx,
            reinterpret_cast<const uint8_t*>(password), pw_len,
            salt, kSaltLen,
            iterations,
            static_cast<uint32_t>(kHashLen), out);
    }
    mbedtls_md_free(&ctx);
#else
    pbkdf2HmacSha256(reinterpret_cast<const uint8_t*>(password), pw_len,
                     salt, kSaltLen, iterations, out);
#endif
}

#ifdef ARDUINO
void generateSalt(uint8_t out[16]) noexcept {
    esp_fill_random(out, kSaltLen);
}
#endif

}  // namespace gh::infra
