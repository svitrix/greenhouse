#include "ArduinoClock.hpp"
#include <Arduino.h>
#include <ctime>

namespace gh::infra {

// Sanity floor for a real wall-clock reading: 2020-09-13T12:26:40Z. A value
// below this means SNTP has not synced yet, so treat the time as unknown.
constexpr time_t kMinValidUnixTimeS = 1'600'000'000;

uint32_t ArduinoClock::nowMs() const noexcept {
    return millis();
}

uint64_t ArduinoClock::unixMs() const noexcept {
    const time_t t = std::time(nullptr);
    if (t < kMinValidUnixTimeS) return 0;
    return static_cast<uint64_t>(t) * 1000ULL;
}
}
