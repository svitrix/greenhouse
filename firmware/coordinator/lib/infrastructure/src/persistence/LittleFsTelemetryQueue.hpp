#pragma once
#include "IBlockFile.hpp"
#include "ports/ITelemetryQueue.hpp"

namespace gh::infra {

// Persistent ring buffer for telemetry records on LittleFS.
//
// File layout:
//   [0]            Header  (magic + version + head + count + CRC32)
//   [kHeaderBytes] N * kRecordWireBytes serialized records
//
// Integrity (E3):
//  - The header carries a magic word, a format version, and a CRC32 over
//    its own fields. A torn/garbage header fails the CRC check on begin()
//    and the queue resets to empty instead of returning garbage telemetry.
//  - Records are serialized field-by-field to a fixed wire width — never a
//    raw memcpy of the padded struct — so the on-flash format is stable and
//    versioned independently of host struct padding.
//  - append() is transactional: the record is written first, and head/count
//    in the header are only published after the record write succeeds, so a
//    failed record write cannot desync the RAM/flash counters.
//
// Authoritative head_/count_ live in RAM (E4); the on-flash header is the
// durable mirror, rewritten after each mutation via a single open handle.
class LittleFsTelemetryQueue final : public gh::domain::ITelemetryQueue {
public:
    static constexpr const char* kPath = "/analytics.bin";

    static constexpr uint32_t kMagic         = 0x47485451;  // "GHTQ"
    static constexpr uint8_t  kFormatVersion = 1;
    static constexpr size_t   kHeaderBytes   = 24;          // see writeHeader_

    static constexpr size_t   kRecordWireBytes = 20;         // see writeRecord_
    static constexpr size_t   kCapBytes        = 192 * 1024; // 192 KiB
    static constexpr size_t   kMaxRecords      = kCapBytes / kRecordWireBytes;

    explicit LittleFsTelemetryQueue(IBlockFile& file) noexcept : file_{file} {}

    // Called once from the composition root. Pre-allocates the backing file
    // on first run, restores head/count, resets on integrity failure.
    [[nodiscard]] gh::domain::ErrorCode begin() noexcept;

    [[nodiscard]] gh::domain::ErrorCode append(const gh::domain::TelemetryRecord& r) noexcept override;
    size_t size() const noexcept override { return count_; }
    size_t peek(gh::domain::TelemetryRecord* out, size_t max) const noexcept override;
    void drop(size_t count) noexcept override;
    void clear() noexcept override;

private:
    bool ensureFile_() noexcept;
    bool readHeader_() noexcept;
    bool writeHeader_() noexcept;
    bool readRecord_(size_t idx, gh::domain::TelemetryRecord& out) const noexcept;
    bool writeRecord_(size_t idx, const gh::domain::TelemetryRecord& r) noexcept;

    IBlockFile& file_;
    size_t      head_  = 0;  // ring index of the oldest record
    size_t      count_ = 0;
};

}  // namespace gh::infra
