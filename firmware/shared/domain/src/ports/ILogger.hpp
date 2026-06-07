#pragma once

namespace gh::domain {
struct ILogger {
    virtual ~ILogger() = default;
    virtual void info (const char* tag, const char* msg) noexcept = 0;
    virtual void warn (const char* tag, const char* msg) noexcept = 0;
    virtual void error(const char* tag, const char* msg) noexcept = 0;
};
}
