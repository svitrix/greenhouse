#pragma once
#include <cstdint>

namespace gh::domain {
struct BatteryReading {
    uint16_t voltage_mv;            // ADC after divider, calibrated
    uint8_t  state_of_charge_pct;   // 0..100
};
}
