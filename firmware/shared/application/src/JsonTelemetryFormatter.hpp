#pragma once
#include <cstddef>
#include <cstdint>
#include "entities/SoilSample.hpp"
#include "entities/AirSample.hpp"
#include "entities/PumpState.hpp"

namespace gh::app::JsonTelemetryFormatter {

[[nodiscard]] int formatSoil(const gh::domain::SoilSample& s,
                              char* buf, size_t bufsize) noexcept;

[[nodiscard]] int formatAir(const gh::domain::AirSample& s,
                             char* buf, size_t bufsize) noexcept;

[[nodiscard]] int formatPump(gh::domain::PumpState st,
                              uint32_t timestamp_ms,
                              char* buf, size_t bufsize) noexcept;

}
