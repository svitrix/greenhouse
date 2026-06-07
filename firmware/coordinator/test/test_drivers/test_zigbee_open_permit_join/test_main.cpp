#include <unity.h>
#include <Arduino.h>

// Placeholder; real assertions land alongside the implementation that uses them.
void test_zigbee_open_permit_join_placeholder() {
    TEST_PASS_MESSAGE("placeholder — to be replaced");
}

void setup() {
    delay(2000);  // USB-CDC settle
    UNITY_BEGIN();
    RUN_TEST(test_zigbee_open_permit_join_placeholder);
    UNITY_END();
}

void loop() {}
