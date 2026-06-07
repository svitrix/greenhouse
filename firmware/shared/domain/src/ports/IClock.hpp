#pragma once
#include <cstdint>

namespace gh::domain {
struct IClock {
    virtual ~IClock() = default;
    [[nodiscard]] virtual uint32_t nowMs() const noexcept = 0;

    // Wall-clock UTC milliseconds since Unix epoch. Returns 0 when SNTP has not
    // synced yet — caller is responsible for the 0-check before recording.
    [[nodiscard]] virtual uint64_t unixMs() const noexcept = 0;
};
}
