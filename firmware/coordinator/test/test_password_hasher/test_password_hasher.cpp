#include <unity.h>
#include <cstring>
#include "ports/IAdminCredsStore.hpp"
#include "platform/PasswordHasher.hpp"

using gh::infra::hashPassword;
using gh::infra::kPbkdf2DefaultIterations;
using gh::domain::AdminCreds;

// 16-byte salt = "salt" + 12 ASCII '0'.
static const uint8_t kSalt0[16] = {
    's','a','l','t','0','0','0','0','0','0','0','0','0','0','0','0'
};

// --- Legacy single SHA-256 (iterations == 0) -------------------------------
// SHA-256(salt0 || "password"), verified via: printf '...' | shasum -a 256.
// Kept so legacy records still verify before they are upgraded.
static const uint8_t kLegacyHash[32] = {
    0x0a, 0xed, 0xea, 0xd8, 0xd4, 0x1a, 0xb0, 0x48,
    0x2c, 0xd0, 0x93, 0x09, 0x32, 0xbf, 0x93, 0x7b,
    0xa2, 0x41, 0x80, 0xef, 0x6c, 0x00, 0xb2, 0x46,
    0xdd, 0x05, 0x1c, 0x0b, 0x98, 0x87, 0x0c, 0xfa,
};

// --- PBKDF2-HMAC-SHA256 vectors (oracle: Python hashlib.pbkdf2_hmac) -------
// pbkdf2(password="password", salt=salt0, iters=1000, dkLen=32)
static const uint8_t kPbkdf2_pw_salt0_1000[32] = {
    0xe8, 0x0f, 0xf5, 0xcf, 0xf8, 0x24, 0x67, 0xbb,
    0xd4, 0xe4, 0x00, 0xf2, 0xd5, 0x86, 0x1d, 0x0e,
    0x53, 0x0c, 0x01, 0x37, 0x3b, 0x9c, 0x54, 0x23,
    0x9b, 0x33, 0xed, 0xeb, 0xd7, 0x15, 0x8c, 0x7c,
};

// pbkdf2(password="password", salt=salt0, iters=1, dkLen=32)
static const uint8_t kPbkdf2_pw_salt0_1[32] = {
    0x64, 0x62, 0x7b, 0x97, 0xf8, 0xc8, 0xbe, 0xc3,
    0xe8, 0xce, 0x1f, 0x45, 0x12, 0xbc, 0x44, 0x6a,
    0x85, 0xb1, 0xee, 0xbb, 0xc8, 0xf2, 0x7d, 0x46,
    0x4c, 0x3c, 0x1b, 0x97, 0xb5, 0xbc, 0x22, 0x00,
};

// pbkdf2(password="abc", salt=0x11*16, iters=4096, dkLen=32)
static const uint8_t kSalt11[16] = {
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
};
static const uint8_t kPbkdf2_abc_salt11_4096[32] = {
    0xc2, 0x74, 0x2f, 0x01, 0xb7, 0x70, 0x21, 0x25,
    0x1d, 0x31, 0x4b, 0x09, 0xfb, 0x93, 0xc2, 0xc9,
    0xa2, 0x14, 0xf3, 0xc9, 0xd1, 0xf3, 0x4a, 0x12,
    0x0f, 0x80, 0x6c, 0x51, 0xb8, 0xfa, 0xfc, 0x59,
};

void test_legacy_single_sha_vector() {
    uint8_t out[32];
    hashPassword("password", kSalt0, /*iterations=*/0, out);
    TEST_ASSERT_EQUAL_MEMORY(kLegacyHash, out, 32);
}

void test_pbkdf2_vector_1000_iters() {
    uint8_t out[32];
    hashPassword("password", kSalt0, /*iterations=*/1000, out);
    TEST_ASSERT_EQUAL_MEMORY(kPbkdf2_pw_salt0_1000, out, 32);
}

void test_pbkdf2_vector_single_iter() {
    uint8_t out[32];
    hashPassword("password", kSalt0, /*iterations=*/1, out);
    TEST_ASSERT_EQUAL_MEMORY(kPbkdf2_pw_salt0_1, out, 32);
}

void test_pbkdf2_vector_4096_iters() {
    uint8_t out[32];
    hashPassword("abc", kSalt11, /*iterations=*/4096, out);
    TEST_ASSERT_EQUAL_MEMORY(kPbkdf2_abc_salt11_4096, out, 32);
}

void test_pbkdf2_differs_from_legacy() {
    uint8_t legacy[32], kdf[32];
    hashPassword("password", kSalt0, 0, legacy);
    hashPassword("password", kSalt0, kPbkdf2DefaultIterations, kdf);
    TEST_ASSERT_FALSE(std::memcmp(legacy, kdf, 32) == 0);
}

void test_iteration_count_changes_hash() {
    uint8_t a[32], b[32];
    hashPassword("password", kSalt0, 1000, a);
    hashPassword("password", kSalt0, 2000, b);
    TEST_ASSERT_FALSE(std::memcmp(a, b, 32) == 0);
}

void test_different_password_different_hash() {
    uint8_t a[32], b[32];
    hashPassword("password1", kSalt0, kPbkdf2DefaultIterations, a);
    hashPassword("password2", kSalt0, kPbkdf2DefaultIterations, b);
    TEST_ASSERT_FALSE(std::memcmp(a, b, 32) == 0);
}

void test_different_salt_different_hash() {
    uint8_t a[32], b[32];
    uint8_t salt1[16]; std::memset(salt1, 0x11, 16);
    uint8_t salt2[16]; std::memset(salt2, 0x22, 16);
    hashPassword("password", salt1, kPbkdf2DefaultIterations, a);
    hashPassword("password", salt2, kPbkdf2DefaultIterations, b);
    TEST_ASSERT_FALSE(std::memcmp(a, b, 32) == 0);
}

void test_empty_password_doesnt_crash() {
    uint8_t out[32];
    hashPassword("", kSalt0, kPbkdf2DefaultIterations, out);
}

// Models the verify-then-upgrade flow in main.cpp's auth callback: a record
// written by the legacy firmware (iterations == 0) still verifies, then is
// re-hashed under the current PBKDF2 default and the new hash verifies too.
void test_migration_legacy_then_upgrade() {
    AdminCreds rec{};
    std::strncpy(rec.username, "admin", AdminCreds::kUsernameMax);
    std::memcpy(rec.salt, kSalt0, 16);
    rec.iterations = AdminCreds::kLegacySingleSha;  // 0
    hashPassword("password", rec.salt, rec.iterations, rec.password_hash);

    // 1) Legacy record verifies against the right password.
    TEST_ASSERT_TRUE(rec.isLegacyFormat());
    uint8_t v[32];
    hashPassword("password", rec.salt, rec.iterations, v);
    TEST_ASSERT_EQUAL_MEMORY(rec.password_hash, v, 32);

    // 2) Wrong password does not verify against the legacy hash.
    hashPassword("wrong", rec.salt, rec.iterations, v);
    TEST_ASSERT_FALSE(std::memcmp(rec.password_hash, v, 32) == 0);

    // 3) Upgrade: re-hash under the PBKDF2 default (same salt).
    AdminCreds upgraded = rec;
    upgraded.iterations = kPbkdf2DefaultIterations;
    hashPassword("password", upgraded.salt, upgraded.iterations, upgraded.password_hash);
    TEST_ASSERT_FALSE(upgraded.isLegacyFormat());
    TEST_ASSERT_FALSE(std::memcmp(rec.password_hash, upgraded.password_hash, 32) == 0);

    // 4) Upgraded record verifies with the same password under the new KDF.
    hashPassword("password", upgraded.salt, upgraded.iterations, v);
    TEST_ASSERT_EQUAL_MEMORY(upgraded.password_hash, v, 32);
}

void setUp(void) {}
void tearDown(void) {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_legacy_single_sha_vector);
    RUN_TEST(test_pbkdf2_vector_1000_iters);
    RUN_TEST(test_pbkdf2_vector_single_iter);
    RUN_TEST(test_pbkdf2_vector_4096_iters);
    RUN_TEST(test_pbkdf2_differs_from_legacy);
    RUN_TEST(test_iteration_count_changes_hash);
    RUN_TEST(test_different_password_different_hash);
    RUN_TEST(test_different_salt_different_hash);
    RUN_TEST(test_empty_password_doesnt_crash);
    RUN_TEST(test_migration_legacy_then_upgrade);
    return UNITY_END();
}
