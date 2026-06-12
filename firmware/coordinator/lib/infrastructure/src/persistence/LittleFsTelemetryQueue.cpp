#include "LittleFsTelemetryQueue.hpp"
#include <cstring>

namespace gh::infra {

using gh::domain::ErrorCode;
using gh::domain::TelemetryKind;
using gh::domain::TelemetryRecord;

namespace {

uint32_t crc32(const uint8_t* data, size_t len) noexcept {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int b = 0; b < 8; ++b) {
            const uint32_t mask = static_cast<uint32_t>(-static_cast<int32_t>(crc & 1u));
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

void putU32(uint8_t* p, uint32_t v) noexcept {
    p[0] = static_cast<uint8_t>(v);
    p[1] = static_cast<uint8_t>(v >> 8);
    p[2] = static_cast<uint8_t>(v >> 16);
    p[3] = static_cast<uint8_t>(v >> 24);
}

uint32_t getU32(const uint8_t* p) noexcept {
    return static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) << 8)
         | (static_cast<uint32_t>(p[2]) << 16)
         | (static_cast<uint32_t>(p[3]) << 24);
}

void putU64(uint8_t* p, uint64_t v) noexcept {
    putU32(p,     static_cast<uint32_t>(v));
    putU32(p + 4, static_cast<uint32_t>(v >> 32));
}

uint64_t getU64(const uint8_t* p) noexcept {
    return static_cast<uint64_t>(getU32(p))
         | (static_cast<uint64_t>(getU32(p + 4)) << 32);
}

}  // namespace

bool LittleFsTelemetryQueue::ensureFile_() noexcept {
    if (file_.exists()) return true;
    if (!file_.create()) return false;

    // Header first, then sparse-zero the data region in large chunks rather
    // than 64-byte writes (the old code issued ~3000 syncs on first boot).
    head_ = 0; count_ = 0;
    if (!writeHeader_()) return false;

    static const uint8_t zbuf[512] = {0};
    size_t to_write = kMaxRecords * kRecordWireBytes;
    size_t offset   = kHeaderBytes;
    while (to_write > 0) {
        const size_t n = (to_write < sizeof(zbuf)) ? to_write : sizeof(zbuf);
        if (file_.writeAt(offset, zbuf, n) != n) return false;
        offset    += n;
        to_write  -= n;
    }
    return true;
}

bool LittleFsTelemetryQueue::writeHeader_() noexcept {
    uint8_t buf[kHeaderBytes] = {0};
    putU32(buf + 0, kMagic);
    buf[4] = kFormatVersion;
    // buf[5..7] reserved
    putU32(buf + 8,  static_cast<uint32_t>(head_));
    putU32(buf + 12, static_cast<uint32_t>(count_));
    // buf[16..19] reserved for future use
    const uint32_t crc = crc32(buf, kHeaderBytes - 4);
    putU32(buf + 20, crc);
    return file_.writeAt(0, buf, kHeaderBytes) == kHeaderBytes;
}

bool LittleFsTelemetryQueue::readHeader_() noexcept {
    uint8_t buf[kHeaderBytes] = {0};
    if (file_.readAt(0, buf, kHeaderBytes) != kHeaderBytes) return false;
    if (getU32(buf + 0) != kMagic)        return false;
    if (buf[4] != kFormatVersion)         return false;
    const uint32_t stored = getU32(buf + 20);
    if (stored != crc32(buf, kHeaderBytes - 4)) return false;

    const uint32_t head  = getU32(buf + 8);
    const uint32_t count = getU32(buf + 12);
    if (count > kMaxRecords) return false;
    head_  = head % kMaxRecords;
    count_ = count;
    return true;
}

bool LittleFsTelemetryQueue::readRecord_(size_t idx, TelemetryRecord& out) const noexcept {
    uint8_t buf[kRecordWireBytes] = {0};
    const size_t off = kHeaderBytes + idx * kRecordWireBytes;
    if (file_.readAt(off, buf, kRecordWireBytes) != kRecordWireBytes) return false;
    out.ts_unix_ms = getU64(buf + 0);
    out.channel_id = buf[8];
    out.kind       = static_cast<TelemetryKind>(buf[9]);
    uint32_t v;  std::memcpy(&v, buf + 10, 4);
    std::memcpy(&out.value, &v, sizeof(float));
    out.raw    = static_cast<int32_t>(getU32(buf + 14));
    out.status = buf[18];
    return true;
}

bool LittleFsTelemetryQueue::writeRecord_(size_t idx, const TelemetryRecord& r) noexcept {
    uint8_t buf[kRecordWireBytes] = {0};
    putU64(buf + 0, r.ts_unix_ms);
    buf[8] = r.channel_id;
    buf[9] = static_cast<uint8_t>(r.kind);
    uint32_t v;  std::memcpy(&v, &r.value, sizeof(float));
    std::memcpy(buf + 10, &v, 4);
    putU32(buf + 14, static_cast<uint32_t>(r.raw));
    buf[18] = r.status;
    const size_t off = kHeaderBytes + idx * kRecordWireBytes;
    return file_.writeAt(off, buf, kRecordWireBytes) == kRecordWireBytes;
}

ErrorCode LittleFsTelemetryQueue::begin() noexcept {
    if (!ensureFile_())  return ErrorCode::QueueIoFailure;
    if (!readHeader_()) {
        // Torn / foreign / corrupt header: reset to a clean empty queue
        // rather than surfacing garbage telemetry.
        head_ = 0; count_ = 0;
        if (!writeHeader_()) return ErrorCode::QueueIoFailure;
    }
    return ErrorCode::Ok;
}

ErrorCode LittleFsTelemetryQueue::append(const TelemetryRecord& r) noexcept {
    // Compute the would-be state without mutating head_/count_ yet, so a
    // failed record write leaves the live counters untouched.
    size_t head = head_;
    size_t count = count_;
    if (count == kMaxRecords) {
        head = (head + 1) % kMaxRecords;
        --count;
    }
    const size_t tail = (head + count) % kMaxRecords;
    if (!writeRecord_(tail, r)) return ErrorCode::QueueIoFailure;

    head_  = head;
    count_ = count + 1;
    if (!writeHeader_()) {
        // Record is durable but the header did not advance; roll RAM back so
        // it matches flash (the record slot is simply overwritten next time).
        head_  = head;
        count_ = count;
        return ErrorCode::QueueIoFailure;
    }
    return ErrorCode::Ok;
}

size_t LittleFsTelemetryQueue::peek(TelemetryRecord* out, size_t max) const noexcept {
    const size_t n = (max < count_) ? max : count_;
    for (size_t i = 0; i < n; ++i) {
        if (!readRecord_((head_ + i) % kMaxRecords, out[i])) return i;
    }
    return n;
}

void LittleFsTelemetryQueue::drop(size_t count) noexcept {
    const size_t n = (count < count_) ? count : count_;
    head_ = (head_ + n) % kMaxRecords;
    count_ -= n;
    writeHeader_();
}

void LittleFsTelemetryQueue::clear() noexcept {
    file_.remove();
    head_ = 0;
    count_ = 0;
    ensureFile_();
}

}  // namespace gh::infra
