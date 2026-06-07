#include "ArduinoClock.hpp"
// Arduino-ESP32 headers (WString.h) have benign -Wconversion issues we
// cannot fix upstream. Suppress only around the SDK include.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include <Arduino.h>
#pragma GCC diagnostic pop
#include <ctime>

namespace gh::infra {
uint32_t ArduinoClock::nowMs() const noexcept {
    return millis();
}

uint64_t ArduinoClock::unixMs() const noexcept {
    const time_t t = std::time(nullptr);
    if (t < static_cast<time_t>(1'600'000'000)) return 0;
    return static_cast<uint64_t>(t) * 1000ULL;
}
}
