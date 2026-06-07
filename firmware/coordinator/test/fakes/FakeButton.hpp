#pragma once
#include "ports/IButton.hpp"

namespace gh::test {
class FakeButton : public gh::domain::IButton {
public:
    bool held = false;
    int  call_count = 0;

    [[nodiscard]] bool holdConfirmed(uint16_t) noexcept override {
        ++call_count;
        return held;
    }
};
}
