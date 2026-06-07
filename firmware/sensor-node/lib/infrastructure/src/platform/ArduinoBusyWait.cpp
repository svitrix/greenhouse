#include "ArduinoBusyWait.hpp"
// Arduino-ESP32 headers (WString.h) have benign -Wconversion issues we
// cannot fix upstream. Suppress only around the SDK include.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include <Arduino.h>
#pragma GCC diagnostic pop

namespace gh::infra {

void ArduinoBusyWait::delayMs(uint16_t ms) noexcept {
    ::delay(ms);
}

}  // namespace gh::infra
