#pragma once
#include "ports/ILogger.hpp"

namespace gh::test {
class FakeLogger : public gh::domain::ILogger {
public:
    int info_count = 0, warn_count = 0, error_count = 0;
    const char* last_warn = "";
    const char* last_error = "";

    void info (const char*, const char*) noexcept override { ++info_count; }
    void warn (const char*, const char* m) noexcept override { ++warn_count; last_warn = m; }
    void error(const char*, const char* m) noexcept override { ++error_count; last_error = m; }
};
}
