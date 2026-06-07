#include <unity.h>
#include <cstring>
#include "WifiProvisioner.hpp"
#include "fakes/FakeWifiCredsStore.hpp"
#include "fakes/FakeButton.hpp"
#include "fakes/FakeWifiSta.hpp"
#include "fakes/FakeProvisioningFlagStore.hpp"
#include "fakes/FakeWifiFailCounterStore.hpp"
#include "fakes/FakeLogger.hpp"

using namespace gh::test;
using gh::app::WifiProvisioner;
using gh::app::SystemMode;
using gh::domain::ErrorCode;
using gh::domain::WifiCreds;

namespace {
WifiCreds validCreds() {
    WifiCreds c{};
    std::strncpy(c.ssid, "TestWifi", sizeof(c.ssid) - 1);
    std::strncpy(c.password, "TestPass", sizeof(c.password) - 1);
    return c;
}
}

void test_button_held_returns_provisioning_even_with_creds() {
    FakeWifiCredsStore store; FakeButton btn; FakeWifiSta sta;
    FakeProvisioningFlagStore flag; FakeWifiFailCounterStore failCounter;
    FakeLogger log;
    btn.held = true;
    store.next_load = {ErrorCode::Ok, validCreds()};
    sta.next_connect_results = {ErrorCode::Ok};
    WifiProvisioner p(store, btn, sta, flag, failCounter, log);
    TEST_ASSERT_EQUAL(static_cast<int>(SystemMode::Provisioning),
                      static_cast<int>(p.bootstrap()));
    TEST_ASSERT_EQUAL_INT(0, sta.connect_calls);
}

void test_empty_nvs_returns_provisioning() {
    FakeWifiCredsStore store; FakeButton btn; FakeWifiSta sta;
    FakeProvisioningFlagStore flag; FakeWifiFailCounterStore failCounter;
    FakeLogger log;
    store.next_load = {ErrorCode::ConfigNotFound, {}};
    WifiProvisioner p(store, btn, sta, flag, failCounter, log);
    TEST_ASSERT_EQUAL(static_cast<int>(SystemMode::Provisioning),
                      static_cast<int>(p.bootstrap()));
    TEST_ASSERT_EQUAL_INT(0, sta.connect_calls);
}

void test_valid_creds_sta_ok_returns_operational() {
    FakeWifiCredsStore store; FakeButton btn; FakeWifiSta sta;
    FakeProvisioningFlagStore flag; FakeWifiFailCounterStore failCounter;
    FakeLogger log;
    store.next_load = {ErrorCode::Ok, validCreds()};
    sta.next_connect_results = {ErrorCode::Ok};
    WifiProvisioner p(store, btn, sta, flag, failCounter, log);
    TEST_ASSERT_EQUAL(static_cast<int>(SystemMode::Operational),
                      static_cast<int>(p.bootstrap()));
    TEST_ASSERT_EQUAL_INT(1, sta.connect_calls);
}

void test_sta_fails_all_retries_returns_provisioning() {
    FakeWifiCredsStore store; FakeButton btn; FakeWifiSta sta;
    FakeProvisioningFlagStore flag; FakeWifiFailCounterStore failCounter;
    FakeLogger log;
    store.next_load = {ErrorCode::Ok, validCreds()};
    sta.next_connect_results = {ErrorCode::WifiConnectFailed,
                                 ErrorCode::WifiConnectFailed,
                                 ErrorCode::WifiConnectFailed};
    WifiProvisioner p(store, btn, sta, flag, failCounter, log, 30'000, 3);
    TEST_ASSERT_EQUAL(static_cast<int>(SystemMode::Provisioning),
                      static_cast<int>(p.bootstrap()));
    TEST_ASSERT_EQUAL_INT(3, sta.connect_calls);
}

void test_sta_succeeds_on_second_attempt_returns_operational() {
    FakeWifiCredsStore store; FakeButton btn; FakeWifiSta sta;
    FakeProvisioningFlagStore flag; FakeWifiFailCounterStore failCounter;
    FakeLogger log;
    store.next_load = {ErrorCode::Ok, validCreds()};
    sta.next_connect_results = {ErrorCode::WifiConnectFailed, ErrorCode::Ok};
    WifiProvisioner p(store, btn, sta, flag, failCounter, log);
    TEST_ASSERT_EQUAL(static_cast<int>(SystemMode::Operational),
                      static_cast<int>(p.bootstrap()));
    TEST_ASSERT_EQUAL_INT(2, sta.connect_calls);
}

void test_invalid_creds_in_nvs_returns_provisioning() {
    FakeWifiCredsStore store; FakeButton btn; FakeWifiSta sta;
    FakeProvisioningFlagStore flag; FakeWifiFailCounterStore failCounter;
    FakeLogger log;
    WifiCreds invalid{};   // empty ssid
    store.next_load = {ErrorCode::Ok, invalid};
    WifiProvisioner p(store, btn, sta, flag, failCounter, log);
    TEST_ASSERT_EQUAL(static_cast<int>(SystemMode::Provisioning),
                      static_cast<int>(p.bootstrap()));
    TEST_ASSERT_EQUAL_INT(0, sta.connect_calls);
}

void test_force_flag_set_enters_provisioning_even_with_creds() {
    FakeWifiCredsStore store; FakeButton btn; FakeWifiSta sta;
    FakeProvisioningFlagStore flag; FakeWifiFailCounterStore failCounter;
    FakeLogger log;
    store.next_load = {ErrorCode::Ok, validCreds()};
    sta.next_connect_results = {ErrorCode::Ok};
    flag.flag = true;
    WifiProvisioner p(store, btn, sta, flag, failCounter, log);
    TEST_ASSERT_EQUAL(static_cast<int>(SystemMode::Provisioning),
                      static_cast<int>(p.bootstrap()));
    TEST_ASSERT_EQUAL_INT(0, sta.connect_calls);  // never attempted STA
}

void test_fail_counter_threshold_returns_provisioning() {
    FakeWifiCredsStore store; FakeButton btn; FakeWifiSta sta;
    FakeProvisioningFlagStore flag; FakeWifiFailCounterStore failCounter;
    FakeLogger log;
    store.next_load = {ErrorCode::Ok, validCreds()};
    sta.next_connect_results = {ErrorCode::Ok};  // would have succeeded
    failCounter.value = 10;  // at threshold

    WifiProvisioner p(store, btn, sta, flag, failCounter, log);
    TEST_ASSERT_EQUAL(static_cast<int>(SystemMode::Provisioning),
                      static_cast<int>(p.bootstrap()));
    TEST_ASSERT_EQUAL_INT(0, sta.connect_calls);  // never even tried
    TEST_ASSERT_EQUAL_INT(1, failCounter.reset_calls);
}

void test_fail_counter_increments_after_failed_retries() {
    FakeWifiCredsStore store; FakeButton btn; FakeWifiSta sta;
    FakeProvisioningFlagStore flag; FakeWifiFailCounterStore failCounter;
    FakeLogger log;
    store.next_load = {ErrorCode::Ok, validCreds()};
    sta.next_connect_results = {ErrorCode::WifiConnectFailed,
                                 ErrorCode::WifiConnectFailed,
                                 ErrorCode::WifiConnectFailed};
    failCounter.value = 3;

    WifiProvisioner p(store, btn, sta, flag, failCounter, log);
    TEST_ASSERT_EQUAL(static_cast<int>(SystemMode::Provisioning),
                      static_cast<int>(p.bootstrap()));
    TEST_ASSERT_EQUAL_INT(1, failCounter.increment_calls);
    TEST_ASSERT_EQUAL_INT(4, failCounter.value);
}

void test_button_held_resets_fail_counter() {
    FakeWifiCredsStore store; FakeButton btn; FakeWifiSta sta;
    FakeProvisioningFlagStore flag; FakeWifiFailCounterStore failCounter;
    FakeLogger log;
    btn.held = true;
    failCounter.value = 7;

    WifiProvisioner p(store, btn, sta, flag, failCounter, log);
    TEST_ASSERT_EQUAL(static_cast<int>(SystemMode::Provisioning),
                      static_cast<int>(p.bootstrap()));
    TEST_ASSERT_EQUAL_INT(1, failCounter.reset_calls);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_button_held_returns_provisioning_even_with_creds);
    RUN_TEST(test_empty_nvs_returns_provisioning);
    RUN_TEST(test_valid_creds_sta_ok_returns_operational);
    RUN_TEST(test_sta_fails_all_retries_returns_provisioning);
    RUN_TEST(test_sta_succeeds_on_second_attempt_returns_operational);
    RUN_TEST(test_invalid_creds_in_nvs_returns_provisioning);
    RUN_TEST(test_force_flag_set_enters_provisioning_even_with_creds);
    RUN_TEST(test_fail_counter_threshold_returns_provisioning);
    RUN_TEST(test_fail_counter_increments_after_failed_retries);
    RUN_TEST(test_button_held_resets_fail_counter);
    return UNITY_END();
}
