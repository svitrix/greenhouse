#include <unity.h>
#include <cstring>
#include "../fakes/FakePairingClient.hpp"

using namespace gh::domain;
using namespace gh::test;

void test_happy_path_fills_api_key() {
    FakePairingClient fake;
    fake.next_result = ErrorCode::Ok;
    fake.next_api_key = "abcdef0123456789abcdef0123456789"
                        "abcdef0123456789abcdef0123456789";

    char api_key[96] = {};
    auto err = fake.claim(
        "http://backend/ingest", "847291", "gh-x1",
        "aa:bb:cc:dd:ee:ff", "0.4.0", "gh-coordinator-v1",
        api_key, sizeof(api_key)
    );
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok), static_cast<int>(err));
    TEST_ASSERT_EQUAL_INT(64, static_cast<int>(std::strlen(api_key)));
}

void test_window_expired_propagates() {
    FakePairingClient fake;
    fake.next_result = ErrorCode::PairingWindowExpired;

    char api_key[96] = {};
    auto err = fake.claim(
        "http://b", "000000", "gh-x", "aa:bb:cc:dd:ee:ff",
        "0.4.0", "gh-coordinator-v1", api_key, sizeof(api_key)
    );
    TEST_ASSERT_EQUAL(
        static_cast<int>(ErrorCode::PairingWindowExpired),
        static_cast<int>(err)
    );
}

void test_profile_unknown_propagates() {
    FakePairingClient fake;
    fake.next_result = ErrorCode::PairingProfileUnknown;

    char api_key[96] = {};
    auto err = fake.claim(
        "http://b", "847291", "gh-x", "aa:bb:cc:dd:ee:ff",
        "9.9.9", "gh-future-v9", api_key, sizeof(api_key)
    );
    TEST_ASSERT_EQUAL(
        static_cast<int>(ErrorCode::PairingProfileUnknown),
        static_cast<int>(err)
    );
}

void test_arguments_are_captured() {
    FakePairingClient fake;
    char api_key[96] = {};
    fake.claim(
        "http://example.com/ingest", "555444", "gh-spec",
        "aa:bb:cc:dd:ee:ff", "1.2.3", "gh-coordinator-v1",
        api_key, sizeof(api_key)
    );
    TEST_ASSERT_EQUAL_STRING("http://example.com/ingest", fake.last_backend_url.c_str());
    TEST_ASSERT_EQUAL_STRING("555444",                    fake.last_claim_code.c_str());
    TEST_ASSERT_EQUAL_STRING("gh-spec",                   fake.last_device_id.c_str());
    TEST_ASSERT_EQUAL_STRING("gh-coordinator-v1",         fake.last_profile_id.c_str());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_happy_path_fills_api_key);
    RUN_TEST(test_window_expired_propagates);
    RUN_TEST(test_profile_unknown_propagates);
    RUN_TEST(test_arguments_are_captured);
    return UNITY_END();
}
