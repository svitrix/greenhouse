#include "StatusBlinker.hpp"

namespace gh::sensor {

StatusBlinker::StatusBlinker(gh::domain::IRgbLed& led,
                             gh::domain::IDelay& delay) noexcept
    : led_(led), delay_(delay) {}

void StatusBlinker::emit(StatusCode code) noexcept {
    const BlinkPattern p = patternFor(code);
    for (uint8_t i = 0; i < p.count; ++i) {
        led_.setColor(p.r, p.g, p.b);
        delay_.delayMs(p.on_ms);
        led_.setColor(0, 0, 0);
        if (p.off_ms != 0) {
            delay_.delayMs(p.off_ms);
        }
    }
}

}  // namespace gh::sensor
