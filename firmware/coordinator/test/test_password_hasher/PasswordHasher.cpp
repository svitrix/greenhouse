// Forwarding translation unit: pulls the PasswordHasher implementation
// into the test_password_hasher binary without requiring the
// Arduino-locked gh-infra-coordinator library. The #ifdef ARDUINO
// guard in PasswordHasher.cpp routes the host build to the vendored
// SHA-256 reference (sha256_ref.h) instead of mbedTLS.
#include "../../lib/infrastructure/src/platform/PasswordHasher.cpp"  // NOLINT(build/include)
