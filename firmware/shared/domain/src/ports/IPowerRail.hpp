#pragma once

namespace gh::domain {

// Abstraction over a switchable power rail (today a single p-MOSFET on
// GPIO4 driven by GpioPowerRail). No implicit warmup — callers compute
// warmup from per-sensor declarations and wait themselves. See spec §3.3.
class IPowerRail {
public:
    virtual ~IPowerRail() = default;
    virtual void on()  noexcept = 0;
    virtual void off() noexcept = 0;
    [[nodiscard]] virtual bool isOn() const noexcept = 0;
};

}
