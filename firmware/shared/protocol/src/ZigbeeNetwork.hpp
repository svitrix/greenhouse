#pragma once
#include <cstdint>
#include "secrets.hpp"

// Network-identity constants shared between coordinator and sensor-node.
// Pinning these makes the pair refuse to join (or be joined by) any other
// Zigbee network. Together with a custom Trust Center link key (see
// secrets.hpp) this prevents both accidental cross-binding to a neighbour's
// hub and sniffer-based decryption of the join handshake.

namespace gh::protocol {

// Compile-time gate against shipping with the all-zero TC link key from
// secrets.hpp.example. A build break is far better than a silent ship
// with the well-known sentinel key.
namespace detail {
constexpr bool kTcLinkKeyHasNonzeroByte = [](){
    for (auto b : kZigbeeTcLinkKey) {
        if (b != 0) return true;
    }
    return false;
}();
}  // namespace detail
static_assert(detail::kTcLinkKeyHasNonzeroByte,
    "kZigbeeTcLinkKey is all-zero — did you copy secrets.hpp.example to "
    "secrets.hpp without filling in real bytes? "
    "Generate with: openssl rand -hex 16");

// 2.4 GHz Zigbee channel. 25 sits above Wi-Fi channels 1/6/11 in most
// jurisdictions and is the quietest in typical households.
constexpr uint8_t  kZigbeeChannel = 25;

// channel_mask form expected by esp_zb_set_primary_network_channel_set().
constexpr uint32_t kZigbeeChannelMask = (1U << kZigbeeChannel);

// DEPRECATED as of 2026-05-31: the coordinator now generates and persists
// its own ExtPanId in NVS namespace "zigbee_net" (see
// firmware/coordinator/.../NvsZigbeeNetStore). The sensor-node no longer
// asserts this value during steering — it attaches by TC link key +
// channel mask. The constant is preserved so existing code that #include's
// this header still compiles, but it is unused in the runtime path.
constexpr uint8_t kZigbeeExtPanId[8] = {
    0x00, 0x01, 'T', 'E', 'N', '-', 'H', 'G'  // reversed: "GH-NET\x01\x00"
};

// Anti-distant-attacker: refuse association from devices whose advertised
// LQI is below this threshold. 32 ≈ -85 dBm in TI-style mapping — far enough
// to cover a normal house, close enough to drop a sniffer behind a wall.
// Set 0 to disable.
constexpr uint8_t kZigbeeMinJoinLqi = 32;

// How long (ms) the coordinator keeps permit-join open on a factory-fresh
// boot (no saved network in NVS). On subsequent boots the window stays
// closed — pairing additional devices requires an explicit open command.
// 254 s (the Zigbee permit-join max) gives a relaxed manual pairing window so
// the operator can power-cycle the sensor-node into steering without racing a
// short timer.
constexpr uint32_t kInitialPermitJoinMs = 254'000;

// Shared WS2812 pattern while a device is ready to pair: slow blue pulse.
// Coordinator shows it while permit-join is open; sensor-node while steering.
constexpr uint8_t  kPairingLedR        = 0;
constexpr uint8_t  kPairingLedG        = 100;
constexpr uint8_t  kPairingLedB        = 255;
constexpr uint16_t kPairingLedPeriodMs = 1000;
constexpr uint16_t kPairingLedOnMs     = 500;

}
