#pragma once
#include "ports/IFloatSwitch.hpp"

namespace gh::test {
class FakeFloatSwitch : public gh::domain::IFloatSwitch {
public:
    bool has_water = true;
    [[nodiscard]] bool hasWater() const noexcept override { return has_water; }
};
}
