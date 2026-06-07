#pragma once
#include "ports/ISystemInfo.hpp"

namespace gh::infra {

class ArduinoSystemInfo final : public gh::domain::ISystemInfo {
public:
    explicit ArduinoSystemInfo(const char* firmware_version) noexcept;
    void snapshot(gh::domain::SystemInfo& out) noexcept override;
private:
    const char* fw_version_;
};

}
