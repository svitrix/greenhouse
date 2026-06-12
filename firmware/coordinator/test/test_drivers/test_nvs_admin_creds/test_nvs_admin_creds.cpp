#include <Arduino.h>
#include <unity.h>
#include <Preferences.h>
#include <cstring>
#include "persistence/NvsAdminCredsStore.hpp"

using gh::infra::NvsAdminCredsStore;
using gh::domain::ErrorCode;
using gh::domain::AdminCreds;

namespace {
void clearNamespace() {
    Preferences p;
    p.begin("admin", false);
    p.remove("user");
    p.remove("pw_hash");
    p.remove("salt");
    p.remove("iter");
    p.end();
}

AdminCreds makeCreds(const char* user, uint8_t hashFill, uint8_t saltFill,
                     uint32_t iterations = 75'000) {
    AdminCreds c{};
    std::strncpy(c.username, user, AdminCreds::kUsernameMax);
    c.username[AdminCreds::kUsernameMax] = '\0';
    std::memset(c.password_hash, hashFill, AdminCreds::kHashLen);
    std::memset(c.salt,          saltFill, AdminCreds::kSaltLen);
    c.iterations = iterations;
    return c;
}
}

void setUp() { clearNamespace(); }
void tearDown() { clearNamespace(); }

void test_load_not_found_when_unset() {
    NvsAdminCredsStore s;
    auto r = s.load();
    TEST_ASSERT_EQUAL(ErrorCode::ConfigNotFound, r.err);
}

void test_save_then_load_roundtrip() {
    NvsAdminCredsStore s;
    auto c = makeCreds("admin", 0xAB, 0xCD);
    TEST_ASSERT_EQUAL(ErrorCode::Ok, s.save(c));
    auto r = s.load();
    TEST_ASSERT_EQUAL(ErrorCode::Ok, r.err);
    TEST_ASSERT_EQUAL_STRING("admin", r.value.username);
    TEST_ASSERT_EQUAL_MEMORY(c.password_hash, r.value.password_hash, AdminCreds::kHashLen);
    TEST_ASSERT_EQUAL_MEMORY(c.salt,          r.value.salt,          AdminCreds::kSaltLen);
    TEST_ASSERT_EQUAL_UINT32(c.iterations, r.value.iterations);
}

// A record written by the legacy firmware has no "iter" key. load()
// must surface iterations == 0 (the legacy single-SHA marker) so the verify
// path can still authenticate and then upgrade it.
void test_legacy_record_loads_with_zero_iterations() {
    {
        Preferences p;
        p.begin("admin", false);
        uint8_t hash[AdminCreds::kHashLen]; std::memset(hash, 0x5A, sizeof(hash));
        uint8_t salt[AdminCreds::kSaltLen]; std::memset(salt, 0x6B, sizeof(salt));
        p.putString("user", "admin");
        p.putBytes("pw_hash", hash, sizeof(hash));
        p.putBytes("salt",    salt, sizeof(salt));
        // intentionally no "iter" key
        p.end();
    }
    NvsAdminCredsStore s;
    auto r = s.load();
    TEST_ASSERT_EQUAL(ErrorCode::Ok, r.err);
    TEST_ASSERT_TRUE(r.value.isLegacyFormat());
    TEST_ASSERT_EQUAL_UINT32(0u, r.value.iterations);
}

void test_save_overwrites_previous() {
    NvsAdminCredsStore s;
    TEST_ASSERT_EQUAL(ErrorCode::Ok, s.save(makeCreds("admin",  0x11, 0x22)));
    TEST_ASSERT_EQUAL(ErrorCode::Ok, s.save(makeCreds("oleksa", 0x33, 0x44)));
    auto r = s.load();
    TEST_ASSERT_EQUAL(ErrorCode::Ok, r.err);
    TEST_ASSERT_EQUAL_STRING("oleksa", r.value.username);
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_load_not_found_when_unset);
    RUN_TEST(test_save_then_load_roundtrip);
    RUN_TEST(test_save_overwrites_previous);
    RUN_TEST(test_legacy_record_loads_with_zero_iterations);
    UNITY_END();
}

void loop() {}
