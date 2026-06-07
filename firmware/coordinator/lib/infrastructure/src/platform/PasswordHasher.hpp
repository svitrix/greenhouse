#pragma once
#include <cstddef>
#include <cstdint>

namespace gh::infra {

// SHA-256(salt || password). salt is 16 bytes, out is 32 bytes.
// Pure function - host-testable.
void hashPassword(const char* password, const uint8_t salt[16], uint8_t out[32]) noexcept;

// 16 cryptographically random bytes via esp_fill_random (hardware RNG).
// On native test env, fills with a deterministic pattern - tests that
// need real randomness must pre-seed via memcpy.
void generateSalt(uint8_t out[16]) noexcept;

}  // namespace gh::infra
