#pragma once
#include "ports/ILogger.hpp"

namespace gh::infra {
class SerialLogger final : public gh::domain::ILogger {
public:
    void info (const char* tag, const char* msg) noexcept override;
    void warn (const char* tag, const char* msg) noexcept override;
    void error(const char* tag, const char* msg) noexcept override;
};
}
