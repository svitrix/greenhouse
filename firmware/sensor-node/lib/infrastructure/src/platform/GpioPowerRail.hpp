#pragma once
#include <cstdint>
#include "errors/ErrorCode.hpp"
#include "ports/IPowerRail.hpp"

namespace gh::infra {

// p-MOSFET driver for the sensor power rail (sensor-node).
// Hardware constraints (GPIO4 strapping pin, 100 kΩ pull-up requirement,
// gpio_hold_en across deep sleep) — unchanged from former SensorPowerGate.
// Implements gh::domain::IPowerRail. No implicit warmup — each
// ISensorChannel declares its own warmupMs() and SensorCycle waits the max.
class GpioPowerRail final : public gh::domain::IPowerRail {
public:
    explicit GpioPowerRail(uint8_t gate_pin) noexcept;

    // Drive safe state + pinMode(OUTPUT) + release any prior deep-sleep hold.
    [[nodiscard]] gh::domain::ErrorCode init() noexcept;

    void on()  noexcept override;   // MOSFET conducting (gate LOW)
    void off() noexcept override;   // MOSFET off (gate HIGH, latched across DS)
    [[nodiscard]] bool isOn() const noexcept override { return is_on_; }

private:
    uint8_t gate_pin_;
    bool    initialised_ = false;
    bool    is_on_       = false;
};

}
