#pragma once
#include "ports/ITelemetrySink.hpp"

namespace gh::test {
class FakeTelemetrySink : public gh::domain::ITelemetrySink {
public:
    int soil_publishes = 0;
    int air_publishes  = 0;
    int pump_publishes = 0;
    gh::domain::SoilSample  last_soil{};
    gh::domain::AirSample   last_air{};
    gh::domain::PumpState   last_pump_state = gh::domain::PumpState::Off;
    gh::domain::ErrorCode   next_error      = gh::domain::ErrorCode::Ok;

    gh::domain::ErrorCode publishSoil(const gh::domain::SoilSample& s) noexcept override {
        ++soil_publishes; last_soil = s; return next_error;
    }
    gh::domain::ErrorCode publishAir(const gh::domain::AirSample& s) noexcept override {
        ++air_publishes; last_air = s; return next_error;
    }
    gh::domain::ErrorCode publishPumpState(gh::domain::PumpState s) noexcept override {
        ++pump_publishes; last_pump_state = s; return next_error;
    }
};
}
