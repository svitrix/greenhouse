#pragma once
#include "ports/IRgbLed.hpp"
#include "ports/IDelay.hpp"
#include "BlinkCodes.hpp"

namespace gh::sensor {

// One-shot, blocking status-LED blinker for the sleepy sensor-node. Emits a
// blink pattern to completion and leaves the LED dark. Intended ONLY for
// terminal pre-sleep paths (join result, rail fault) — the blocking waits would
// violate the no-delay-in-hot-path rule anywhere else. The wait is injected via
// IDelay so it is a no-op in host tests (which assert the colour sequence).
class StatusBlinker {
public:
    StatusBlinker(gh::domain::IRgbLed& led, gh::domain::IDelay& delay) noexcept;

    // Blocks until the pattern completes; ends with the LED off.
    void emit(StatusCode code) noexcept;

private:
    gh::domain::IRgbLed& led_;
    gh::domain::IDelay&  delay_;
};

}
