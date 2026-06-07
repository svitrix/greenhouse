#pragma once
#include "entities/SoilSample.hpp"
#include "util/Result.hpp"

namespace gh::test {
class FakeSoilSensor {
public:
    gh::domain::Result<gh::domain::SoilSample> next{
        gh::domain::ErrorCode::Ok, {0, 500, 0, 220}};
    int read_calls = 0;

    [[nodiscard]] gh::domain::Result<gh::domain::SoilSample> read() noexcept {
        ++read_calls;
        return next;
    }
};
}
