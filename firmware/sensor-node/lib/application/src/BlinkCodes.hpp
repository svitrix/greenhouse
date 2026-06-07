#pragma once
#include <cstdint>
#include "errors/ErrorCode.hpp"
#include "ZigbeeNetwork.hpp"

namespace gh::sensor {

// Operator-facing status codes shown on the on-board WS2812 right before the
// node deep-sleeps. The LED is the sensor-node's only reliable signal in
// production (no USB-CDC, app-logs go to an unexposed UART). The two failure
// modes differ in BLINK COUNT as well as hue — fine hue discrimination on a
// tiny WS2812 at low brightness is unreliable, so the count carries the signal.
enum class StatusCode : uint8_t {
    JoinedOk,        // 1x short green — joined + reported this cycle
    PairingReady,    // slow blue pulse — steering, waiting for coordinator
    JoinTimeout,     // 3x red — reached the air, no coordinator (open permit-join)
    StackInitFailed, // 2x magenta — esp_zb stack/cluster init failed (re-flash)
    TcMismatch,      // 1x long amber — TC key rejected, pairing wiped
    RailInitFailed,  // 1x long red (solid) — sensor-rail hardware fault
};

// Full-range colour + blink shape. off_ms == 0 on a single-blink pattern means
// "solid for on_ms, then dark".
struct BlinkPattern {
    uint8_t  r;
    uint8_t  g;
    uint8_t  b;
    uint8_t  count;
    uint16_t on_ms;
    uint16_t off_ms;
};

// Colours mirror the coordinator's StatusLedService language (green=ok,
// red=fault, amber=attention).
[[nodiscard]] constexpr BlinkPattern patternFor(StatusCode code) noexcept {
    switch (code) {
        case StatusCode::JoinedOk:        return {0,   255, 0,   1, 200,  0};
        case StatusCode::PairingReady:   return {
            gh::protocol::kPairingLedR,
            gh::protocol::kPairingLedG,
            gh::protocol::kPairingLedB,
            255,  // continuous tick handled by adapter; unused by emit()
            gh::protocol::kPairingLedOnMs,
            static_cast<uint16_t>(
                gh::protocol::kPairingLedPeriodMs - gh::protocol::kPairingLedOnMs),
        };
        case StatusCode::JoinTimeout:     return {255, 0,   0,   3, 150,  200};
        case StatusCode::StackInitFailed: return {255, 0,   255, 2, 150,  200};
        case StatusCode::TcMismatch:      return {255, 170, 0,   1, 800,  0};
        case StatusCode::RailInitFailed:  return {255, 0,   0,   1, 1000, 0};
    }
    return {0, 0, 0, 0, 0, 0};
}

// Maps a ZigbeeEndDevice::start() result to the status the operator should see.
// Unknown/unexpected codes fall back to StackInitFailed ("firmware problem").
[[nodiscard]] constexpr StatusCode
statusForJoinResult(gh::domain::ErrorCode e) noexcept {
    switch (e) {
        case gh::domain::ErrorCode::Ok:
            return StatusCode::JoinedOk;
        case gh::domain::ErrorCode::ZigbeeJoinTimeout:
            return StatusCode::JoinTimeout;
        case gh::domain::ErrorCode::ZigbeeTrustCenterMismatch:
            return StatusCode::TcMismatch;
        case gh::domain::ErrorCode::ZigbeeStackInitFailed:
            return StatusCode::StackInitFailed;
        default:
            return StatusCode::StackInitFailed;
    }
}

}  // namespace gh::sensor
