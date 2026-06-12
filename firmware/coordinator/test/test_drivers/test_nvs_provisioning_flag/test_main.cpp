#include <Arduino.h>
#include <unity.h>
#include <Preferences.h>
#include "persistence/NvsProvisioningFlagStore.hpp"

using gh::infra::NvsProvisioningFlagStore;
using gh::domain::ErrorCode;

namespace {
void clearNamespace() {
    Preferences p;
    p.begin("prov_flag", false);
    p.clear();
    p.end();
}
}

void setUp() { clearNamespace(); }
void tearDown() { clearNamespace(); }

void test_default_false_on_first_boot() {
    NvsProvisioningFlagStore s;
    TEST_ASSERT_FALSE(s.isForced());
}

void test_set_true_persists() {
    NvsProvisioningFlagStore s;
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(s.setForced(true)));
    TEST_ASSERT_TRUE(s.isForced());
}

void test_set_false_clears() {
    NvsProvisioningFlagStore s;
    (void)s.setForced(true);
    (void)s.setForced(false);
    TEST_ASSERT_FALSE(s.isForced());
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_default_false_on_first_boot);
    RUN_TEST(test_set_true_persists);
    RUN_TEST(test_set_false_clears);
    UNITY_END();
}
void loop() {}
