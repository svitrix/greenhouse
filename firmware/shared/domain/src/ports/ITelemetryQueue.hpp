#pragma once
#include <cstddef>
#include "entities/TelemetryRecord.hpp"
#include "errors/ErrorCode.hpp"

namespace gh::domain {

struct ITelemetryQueue {
    virtual ~ITelemetryQueue() = default;

    // Persist one record. May drop the oldest record if at capacity.
    [[nodiscard]] virtual ErrorCode append(const TelemetryRecord& r) noexcept = 0;

    // Number of records currently held.
    [[nodiscard]] virtual size_t size() const noexcept = 0;

    // Copy up to `max` oldest records into `out`. Returns count copied.
    // Does NOT remove them.
    [[nodiscard]] virtual size_t peek(TelemetryRecord* out, size_t max) const noexcept = 0;

    // Remove the first `count` records (the oldest). No-op if count > size().
    virtual void drop(size_t count) noexcept = 0;

    // Wipe all stored records (debug / factory reset).
    virtual void clear() noexcept = 0;
};

}
