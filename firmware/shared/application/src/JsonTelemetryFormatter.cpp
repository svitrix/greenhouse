#include "JsonTelemetryFormatter.hpp"
#include <cstdio>

namespace gh::app::JsonTelemetryFormatter {

namespace {
const char* pumpStateString(gh::domain::PumpState st) noexcept {
    switch (st) {
        case gh::domain::PumpState::Off:          return "OFF";
        case gh::domain::PumpState::On:           return "ON";
        case gh::domain::PumpState::SafetyLocked: return "LOCKED";
    }
    return "UNKNOWN";
}

int finalize(int n, size_t bufsize) noexcept {
    if (n < 0) return -1;
    if (static_cast<size_t>(n) >= bufsize) return -1;
    return n;
}
}

int formatSoil(const gh::domain::SoilSample& s,
               char* buf, size_t bufsize) noexcept {
    const int n = std::snprintf(
        buf, bufsize,
        "{\"t\":%lu,\"soil\":{\"raw\":%u,\"pct\":%u,\"temp_c10\":%d}}\n",
        static_cast<unsigned long>(s.timestamp_ms),
        static_cast<unsigned>(s.raw_capacitance),
        static_cast<unsigned>(s.moisture_pct),
        static_cast<int>(s.temperature_c_x10));
    return finalize(n, bufsize);
}

int formatAir(const gh::domain::AirSample& s,
              char* buf, size_t bufsize) noexcept {
    const int n = std::snprintf(
        buf, bufsize,
        "{\"t\":%lu,\"air\":{\"temp_c10\":%d,\"rh_x10\":%u}}\n",
        static_cast<unsigned long>(s.timestamp_ms),
        static_cast<int>(s.temperature_c_x10),
        static_cast<unsigned>(s.humidity_pct_x10));
    return finalize(n, bufsize);
}

int formatPump(gh::domain::PumpState st,
               uint32_t timestamp_ms,
               char* buf, size_t bufsize) noexcept {
    const int n = std::snprintf(
        buf, bufsize,
        "{\"t\":%lu,\"pump\":\"%s\"}\n",
        static_cast<unsigned long>(timestamp_ms),
        pumpStateString(st));
    return finalize(n, bufsize);
}

}
