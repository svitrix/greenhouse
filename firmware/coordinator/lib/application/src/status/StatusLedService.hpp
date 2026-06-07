#pragma once
#include <cstdint>
#include "ports/IRgbLed.hpp"
#include "entities/PumpState.hpp"
#include "SystemStatus.hpp"

namespace gh::app {

// Maps a SystemStatus to a colour + blink pattern and drives an IRgbLed.
// Pure logic (no Arduino) so it is exercised in the native test env; the
// hardware lives behind IRgbLed. The owner sets the status (directly, or via
// arbitrate() over runtime signals) and calls tick() periodically to advance
// the blink phase.
class StatusLedService {
public:
    explicit StatusLedService(gh::domain::IRgbLed& led) noexcept;

    void setStatus(SystemStatus s) noexcept { current_ = s; }
    [[nodiscard]] SystemStatus status() const noexcept { return current_; }

    // Advance the blink phase for `now_ms` and write the LED. Idempotent: only
    // touches the LED when the visible state (status + on/off phase) changes.
    void tick(uint32_t now_ms) noexcept;

    // Highest-priority status from operational signals.
    // Fault > Watering > Degraded(Wi-Fi down OR MQTT expected-but-down) > Online.
    // Green (Online) therefore means the Wi-Fi link is actually up — not just
    // "the pump is idle".
    [[nodiscard]] static SystemStatus arbitrate(gh::domain::PumpState pump,
                                                bool wifi_connected,
                                                bool mqtt_connected,
                                                bool mqtt_expected,
                                                bool zigbee_pairing_open = false) noexcept;

private:
    gh::domain::IRgbLed& led_;
    SystemStatus         current_     = SystemStatus::Boot;
    SystemStatus         last_status_ = SystemStatus::Boot;
    bool                 last_on_     = false;
    bool                 written_     = false;
};

}
