#pragma once
#include "ports/ITelemetryQueue.hpp"

namespace gh::infra {

// Persistent ring buffer for telemetry records on LittleFS.
//
// File layout: [u32 head][u32 count][N * sizeof(TelemetryRecord)].
// The data region is pre-allocated to kMaxRecords slots; head/count
// describe the live window over that region. Records survive reboots.
//
// Capacity: 192 KiB cap → ~kMaxRecords slots (depends on record size).
// On overflow the oldest record is dropped (head advances by one).
class LittleFsTelemetryQueue final : public gh::domain::ITelemetryQueue {
public:
    static constexpr const char* kPath        = "/analytics.bin";
    static constexpr size_t      kRecordBytes = sizeof(gh::domain::TelemetryRecord);
    static constexpr size_t      kCapBytes    = 192 * 1024;  // 192 KiB
    static constexpr size_t      kMaxRecords  = kCapBytes / kRecordBytes;

    // Called once from the composition root. Mounts LittleFS if needed,
    // pre-allocates the backing file on first run, restores head/count.
    [[nodiscard]] gh::domain::ErrorCode begin() noexcept;

    [[nodiscard]] gh::domain::ErrorCode append(const gh::domain::TelemetryRecord& r) noexcept override;
    size_t size() const noexcept override { return count_; }
    size_t peek(gh::domain::TelemetryRecord* out, size_t max) const noexcept override;
    void drop(size_t count) noexcept override;
    void clear() noexcept override;

private:
    static constexpr size_t kHeaderBytes = 8;  // u32 head + u32 count

    bool   ensureFile_() noexcept;
    bool   readHeader_() noexcept;
    bool   writeHeader_() noexcept;
    bool   readRecord_(size_t idx, gh::domain::TelemetryRecord& out) const noexcept;
    bool   writeRecord_(size_t idx, const gh::domain::TelemetryRecord& r) noexcept;

    size_t head_  = 0;   // ring index of the oldest record
    size_t count_ = 0;
};

}
