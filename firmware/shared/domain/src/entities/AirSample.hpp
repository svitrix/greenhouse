#pragma once
#include <cstdint>

namespace gh::domain {
struct AirSample {
    uint32_t timestamp_ms;
    int16_t  temperature_c_x10;  // ×10 to avoid float in hot path
    uint16_t humidity_pct_x10;   // 0..1000 (0..100.0% × 10)
};
}
