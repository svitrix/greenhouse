#pragma once
#include <cstddef>
#include <cstdint>

namespace gh::infra {

// Default PBKDF2-HMAC-SHA256 work factor for newly hashed admin passwords.
//
// This is a DELIBERATELY CONSERVATIVE starting value. The ESP32-C6 is a
// single-core 160 MHz RISC-V part; the original review note ("100k ~= 250 ms")
// is optimistic for this chip. 75k is inside the recommended 50k-100k band and
// MUST be re-measured on hardware: the verify runs synchronously inside the
// HTTP basic-auth callback, so the per-attempt cost is also the latency a
// legitimate operator pays on every /api/* request. If a hardware measurement
// shows the verify taking longer than ~400 ms, lower this; if much faster,
// raise it. Bumping this constant alone migrates existing records on next
// successful login (the verify path re-hashes when the stored count differs).
constexpr uint32_t kPbkdf2DefaultIterations = 75'000;

// Derive a 32-byte key from `password` and the 16-byte `salt`.
//
//   - iterations == 0  -> legacy single SHA-256(salt || password). Kept ONLY
//                         so records written by the pre-remediation firmware
//                         still verify (and then get upgraded on login). Never
//                         pass 0 when hashing a NEW password.
//   - iterations >  0  -> PBKDF2-HMAC-SHA256(password, salt, iterations).
//
// Pure function - the same (password, salt, iterations) always yields the same
// `out`. Host-testable: the native build derives the value via a vendored
// SHA-256 reference, the firmware build via mbedTLS.
void hashPassword(const char* password,
                  const uint8_t salt[16],
                  uint32_t iterations,
                  uint8_t out[32]) noexcept;

// Fill `out` with 16 cryptographically random bytes via esp_fill_random
// (hardware RNG). ONLY available on the firmware (ARDUINO) build.
//
// INVARIANT: on a non-ARDUINO build there is no secure RNG here, so this symbol
// is intentionally NOT defined (see PasswordHasher.cpp). A host/test build that
// needs salt bytes must supply them itself (e.g. memcpy a fixed pattern).
// Linking a real firmware without ARDUINO defined fails at compile time via the
// static_assert in the .cpp, so a deterministic salt can never leak into a
// shipped build.
#ifdef ARDUINO
void generateSalt(uint8_t out[16]) noexcept;
#endif

}  // namespace gh::infra
