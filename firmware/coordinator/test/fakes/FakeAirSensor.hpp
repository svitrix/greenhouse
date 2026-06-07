#pragma once
#include "entities/AirSample.hpp"
#include "util/Result.hpp"

namespace gh::test {
class FakeAirSensor {
public:
    gh::domain::Result<gh::domain::AirSample> next{
        gh::domain::ErrorCode::Ok, {0, 240, 555}};
    int read_calls = 0;

    [[nodiscard]] gh::domain::Result<gh::domain::AirSample> read() noexcept {
        ++read_calls;
        return next;
    }
};
}
