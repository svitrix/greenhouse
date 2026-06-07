#pragma once
#include <cstdint>

namespace gh::domain {
struct SoilSample {
    uint32_t timestamp_ms;
    uint16_t raw_capacitance;    // ~200..1000, Chirp 10-bit ADC: 1023 - (caph - capl)
    uint8_t  moisture_pct;       // 0..100, filled by SoilNormalizer (driver writes 0)
    int16_t  temperature_c_x10;  // -200..+850 (Chirp thermistor diapason × 10)
};
}
