#include "StatusLedService.hpp"
#include "ZigbeeNetwork.hpp"

namespace gh::app {
namespace {

// Full-range colour + blink shape per status. period_ms == 0 means solid.
struct Style {
    uint8_t  r;
    uint8_t  g;
    uint8_t  b;
    uint16_t period_ms;  // 0 == solid
    uint16_t on_ms;      // lit fraction of the period
};

constexpr Style styleFor(SystemStatus s) noexcept {
    switch (s) {
        case SystemStatus::Boot:           return {60,  60,  60,     0,   0};  // dim white, solid
        case SystemStatus::Provisioning:   return {0,   0,   255, 1500, 750};  // blue, slow blink
        case SystemStatus::WifiConnecting: return {255, 170, 0,    600, 300};  // amber, blink
        case SystemStatus::Online:         return {0,   255, 0,      0,   0};  // green, solid
        case SystemStatus::Degraded:       return {255, 70,  0,      0,   0};  // orange, solid
        case SystemStatus::Watering:       return {0,   200, 200,    0,   0};  // cyan, solid
        case SystemStatus::Fault:          return {255, 0,   0,    300, 150};  // red, fast blink
        case SystemStatus::ZigbeePairing:  return {
            gh::protocol::kPairingLedR,
            gh::protocol::kPairingLedG,
            gh::protocol::kPairingLedB,
            gh::protocol::kPairingLedPeriodMs,
            gh::protocol::kPairingLedOnMs,
        };
    }
    return {0, 0, 0, 0, 0};
}

}  // namespace

StatusLedService::StatusLedService(gh::domain::IRgbLed& led) noexcept : led_(led) {}

void StatusLedService::tick(uint32_t now_ms) noexcept {
    const Style st = styleFor(current_);
    const bool  on = (st.period_ms == 0) ? true
                                         : ((now_ms % st.period_ms) < st.on_ms);

    if (written_ && current_ == last_status_ && on == last_on_) {
        return;
    }
    if (on) {
        led_.setColor(st.r, st.g, st.b);
    } else {
        led_.setColor(0, 0, 0);
    }
    last_status_ = current_;
    last_on_     = on;
    written_     = true;
}

SystemStatus StatusLedService::arbitrate(gh::domain::PumpState pump,
                                         bool wifi_connected,
                                         bool mqtt_connected,
                                         bool mqtt_expected,
                                         bool zigbee_pairing_open) noexcept {
    if (pump == gh::domain::PumpState::SafetyLocked) return SystemStatus::Fault;
    if (pump == gh::domain::PumpState::On)           return SystemStatus::Watering;
    if (zigbee_pairing_open)                         return SystemStatus::ZigbeePairing;
    if (!wifi_connected)                             return SystemStatus::Degraded;
    if (mqtt_expected && !mqtt_connected)            return SystemStatus::Degraded;
    return SystemStatus::Online;
}

}  // namespace gh::app
