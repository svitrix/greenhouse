#include <unity.h>
#include <cstring>
#include "platform/PasswordHasher.hpp"

using gh::infra::hashPassword;

// Computed via: printf 'salt000000000000password' | shasum -a 256
//   -> 0aedead8d41ab0482cd0930932bf937ba24180ef6c00b246dd051c0b98870cfa
// Note: the 16-byte salt is exactly "salt" + 12 zero ASCII chars
// (kSalt0 below), concatenated with the 8-byte ASCII "password".
static const uint8_t kKnownHash[32] = {
    0x0a, 0xed, 0xea, 0xd8, 0xd4, 0x1a, 0xb0, 0x48,
    0x2c, 0xd0, 0x93, 0x09, 0x32, 0xbf, 0x93, 0x7b,
    0xa2, 0x41, 0x80, 0xef, 0x6c, 0x00, 0xb2, 0x46,
    0xdd, 0x05, 0x1c, 0x0b, 0x98, 0x87, 0x0c, 0xfa,
};

static const uint8_t kSalt0[16] = {
    's','a','l','t','0','0','0','0','0','0','0','0','0','0','0','0'
};

void test_known_vector() {
    uint8_t out[32];
    hashPassword("password", kSalt0, out);
    TEST_ASSERT_EQUAL_MEMORY(kKnownHash, out, 32);
}

void test_different_password_different_hash() {
    uint8_t a[32], b[32];
    hashPassword("password1", kSalt0, a);
    hashPassword("password2", kSalt0, b);
    TEST_ASSERT_FALSE(std::memcmp(a, b, 32) == 0);
}

void test_different_salt_different_hash() {
    uint8_t a[32], b[32];
    uint8_t salt1[16]; std::memset(salt1, 0x11, 16);
    uint8_t salt2[16]; std::memset(salt2, 0x22, 16);
    hashPassword("password", salt1, a);
    hashPassword("password", salt2, b);
    TEST_ASSERT_FALSE(std::memcmp(a, b, 32) == 0);
}

void test_empty_password_doesnt_crash() {
    uint8_t out[32];
    hashPassword("", kSalt0, out);
}

void setUp(void) {}
void tearDown(void) {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_known_vector);
    RUN_TEST(test_different_password_different_hash);
    RUN_TEST(test_different_salt_different_hash);
    RUN_TEST(test_empty_password_doesnt_crash);
    return UNITY_END();
}
