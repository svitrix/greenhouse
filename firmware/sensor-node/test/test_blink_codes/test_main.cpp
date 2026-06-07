#include <unity.h>
#include "BlinkCodes.hpp"
#include "StatusBlinker.hpp"
#include "ZigbeeNetwork.hpp"
#include "fakes/FakeRgbLed.hpp"
#include "fakes/FakeDelay.hpp"

using gh::domain::ErrorCode;
using gh::sensor::BlinkPattern;
using gh::sensor::patternFor;
using gh::sensor::StatusBlinker;
using gh::sensor::StatusCode;
using gh::sensor::statusForJoinResult;

// Unity TEST_ASSERT_EQUAL can't take an enum class directly; compare underlying.
static int code(StatusCode s) { return static_cast<int>(s); }

// --- statusForJoinResult mapping ---------------------------------------------

void test_statusForJoinResult_maps_every_known_code(void) {
    TEST_ASSERT_EQUAL_INT(code(StatusCode::JoinedOk),
                          code(statusForJoinResult(ErrorCode::Ok)));
    TEST_ASSERT_EQUAL_INT(code(StatusCode::JoinTimeout),
                          code(statusForJoinResult(ErrorCode::ZigbeeJoinTimeout)));
    TEST_ASSERT_EQUAL_INT(code(StatusCode::StackInitFailed),
                          code(statusForJoinResult(ErrorCode::ZigbeeStackInitFailed)));
    TEST_ASSERT_EQUAL_INT(code(StatusCode::TcMismatch),
                          code(statusForJoinResult(ErrorCode::ZigbeeTrustCenterMismatch)));
}

void test_statusForJoinResult_unknown_falls_back_to_stack_init(void) {
    TEST_ASSERT_EQUAL_INT(code(StatusCode::StackInitFailed),
                          code(statusForJoinResult(ErrorCode::NetworkDown)));
    TEST_ASSERT_EQUAL_INT(code(StatusCode::StackInitFailed),
                          code(statusForJoinResult(ErrorCode::Timeout)));
}

// --- patternFor: distinct, bounded -------------------------------------------

void test_patternFor_distinguishes_failure_modes_by_count_and_hue(void) {
    const BlinkPattern timeout = patternFor(StatusCode::JoinTimeout);
    const BlinkPattern stack   = patternFor(StatusCode::StackInitFailed);
    TEST_ASSERT_EQUAL_UINT8(3, timeout.count);   // red x3
    TEST_ASSERT_EQUAL_UINT8(2, stack.count);     // magenta x2 — distinct count
    TEST_ASSERT_EQUAL_UINT8(0,   timeout.b);     // red:     blue=0
    TEST_ASSERT_EQUAL_UINT8(255, stack.b);       // magenta: blue=255
}

void test_patternFor_joinedok_is_single_green(void) {
    const BlinkPattern ok = patternFor(StatusCode::JoinedOk);
    TEST_ASSERT_EQUAL_UINT8(1,   ok.count);
    TEST_ASSERT_EQUAL_UINT8(0,   ok.r);
    TEST_ASSERT_EQUAL_UINT8(255, ok.g);
    TEST_ASSERT_EQUAL_UINT8(0,   ok.b);
}

void test_patternFor_pairing_ready_matches_shared_protocol(void) {
    const BlinkPattern ready = patternFor(StatusCode::PairingReady);
    TEST_ASSERT_EQUAL_UINT8(gh::protocol::kPairingLedR, ready.r);
    TEST_ASSERT_EQUAL_UINT8(gh::protocol::kPairingLedG, ready.g);
    TEST_ASSERT_EQUAL_UINT8(gh::protocol::kPairingLedB, ready.b);
    TEST_ASSERT_EQUAL_UINT16(gh::protocol::kPairingLedOnMs, ready.on_ms);
}

// --- StatusBlinker emits the right sequence and ends dark --------------------

void test_blinker_emits_count_pulses_and_ends_off(void) {
    gh::test::FakeRgbLed led;
    gh::test::FakeDelay  delay;
    StatusBlinker blinker{led, delay};

    blinker.emit(StatusCode::JoinTimeout);  // red x3

    // 3 pulses => 6 writes: (on, off) x3.
    TEST_ASSERT_EQUAL_INT(6, led.count);
    for (int i = 0; i < 3; ++i) {
        const auto on  = led.writes[i * 2];
        const auto off = led.writes[i * 2 + 1];
        TEST_ASSERT_EQUAL_UINT8(255, on.r);
        TEST_ASSERT_EQUAL_UINT8(0,   on.g);
        TEST_ASSERT_EQUAL_UINT8(0,   on.b);
        TEST_ASSERT_EQUAL_UINT8(0, off.r);
        TEST_ASSERT_EQUAL_UINT8(0, off.g);
        TEST_ASSERT_EQUAL_UINT8(0, off.b);
    }
    // 3 on-delays + 3 off-delays.
    TEST_ASSERT_EQUAL_INT(6, delay.calls);
}

void test_blinker_solid_pattern_is_one_on_then_off_no_trailing_delay(void) {
    gh::test::FakeRgbLed led;
    gh::test::FakeDelay  delay;
    StatusBlinker blinker{led, delay};

    blinker.emit(StatusCode::RailInitFailed);  // red solid: count 1, off_ms 0

    // count 1 => on + off = 2 writes; ends off.
    TEST_ASSERT_EQUAL_INT(2, led.count);
    TEST_ASSERT_EQUAL_UINT8(255, led.writes[0].r);
    TEST_ASSERT_EQUAL_UINT8(0,   led.writes[1].r);
    TEST_ASSERT_EQUAL_UINT8(0,   led.writes[1].g);
    TEST_ASSERT_EQUAL_UINT8(0,   led.writes[1].b);
    // one on-delay; no off-delay because off_ms == 0.
    TEST_ASSERT_EQUAL_INT(1, delay.calls);
}

void setUp() {}
void tearDown() {}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_statusForJoinResult_maps_every_known_code);
    RUN_TEST(test_statusForJoinResult_unknown_falls_back_to_stack_init);
    RUN_TEST(test_patternFor_distinguishes_failure_modes_by_count_and_hue);
    RUN_TEST(test_patternFor_joinedok_is_single_green);
    RUN_TEST(test_patternFor_pairing_ready_matches_shared_protocol);
    RUN_TEST(test_blinker_emits_count_pulses_and_ends_off);
    RUN_TEST(test_blinker_solid_pattern_is_one_on_then_off_no_trailing_delay);
    return UNITY_END();
}
