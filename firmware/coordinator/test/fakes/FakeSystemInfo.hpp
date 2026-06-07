#pragma once
#include "ports/ISystemInfo.hpp"

namespace gh::test {

class FakeSystemInfo final : public gh::domain::ISystemInfo {
public:
    gh::domain::SystemInfo s{};
    void snapshot(gh::domain::SystemInfo& out) noexcept override { out = s; }
};

}
