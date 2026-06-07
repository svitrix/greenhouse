#include "LittleFsTelemetryQueue.hpp"
#include <LittleFS.h>
#include <cstdint>

namespace gh::infra {

using gh::domain::ErrorCode;
using gh::domain::TelemetryRecord;

bool LittleFsTelemetryQueue::ensureFile_() noexcept {
    if (LittleFS.exists(kPath)) return true;
    File f = LittleFS.open(kPath, "w");
    if (!f) return false;
    uint32_t zero[2] = {0, 0};
    f.write(reinterpret_cast<uint8_t*>(zero), kHeaderBytes);
    // Pre-allocate the data region with zeros so subsequent seeks work.
    static const uint8_t zbuf[64] = {0};
    size_t to_write = kMaxRecords * kRecordBytes;
    while (to_write > 0) {
        const size_t n = (to_write < sizeof(zbuf)) ? to_write : sizeof(zbuf);
        f.write(zbuf, n);
        to_write -= n;
    }
    f.close();
    return true;
}

bool LittleFsTelemetryQueue::readHeader_() noexcept {
    File f = LittleFS.open(kPath, "r");
    if (!f) return false;
    uint32_t hdr[2] = {0, 0};
    f.read(reinterpret_cast<uint8_t*>(hdr), kHeaderBytes);
    f.close();
    head_  = hdr[0] % kMaxRecords;
    count_ = (hdr[1] > kMaxRecords) ? kMaxRecords : hdr[1];
    return true;
}

bool LittleFsTelemetryQueue::writeHeader_() noexcept {
    File f = LittleFS.open(kPath, "r+");
    if (!f) return false;
    uint32_t hdr[2] = {static_cast<uint32_t>(head_), static_cast<uint32_t>(count_)};
    f.seek(0);
    f.write(reinterpret_cast<uint8_t*>(hdr), kHeaderBytes);
    f.close();
    return true;
}

bool LittleFsTelemetryQueue::readRecord_(size_t idx, TelemetryRecord& out) const noexcept {
    File f = LittleFS.open(kPath, "r");
    if (!f) return false;
    if (!f.seek(kHeaderBytes + idx * kRecordBytes)) { f.close(); return false; }
    const size_t n = f.read(reinterpret_cast<uint8_t*>(&out), kRecordBytes);
    f.close();
    return n == kRecordBytes;
}

bool LittleFsTelemetryQueue::writeRecord_(size_t idx, const TelemetryRecord& r) noexcept {
    File f = LittleFS.open(kPath, "r+");
    if (!f) return false;
    if (!f.seek(kHeaderBytes + idx * kRecordBytes)) { f.close(); return false; }
    const size_t n = f.write(reinterpret_cast<const uint8_t*>(&r), kRecordBytes);
    f.close();
    return n == kRecordBytes;
}

ErrorCode LittleFsTelemetryQueue::begin() noexcept {
    if (!LittleFS.begin(true)) return ErrorCode::FsMountFailed;
    if (!ensureFile_())        return ErrorCode::QueueIoFailure;
    if (!readHeader_())        return ErrorCode::QueueIoFailure;
    return ErrorCode::Ok;
}

ErrorCode LittleFsTelemetryQueue::append(const TelemetryRecord& r) noexcept {
    if (count_ == kMaxRecords) {
        head_ = (head_ + 1) % kMaxRecords;
        --count_;
    }
    const size_t tail = (head_ + count_) % kMaxRecords;
    if (!writeRecord_(tail, r)) return ErrorCode::QueueIoFailure;
    ++count_;
    if (!writeHeader_())        return ErrorCode::QueueIoFailure;
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
    LittleFS.remove(kPath);
    head_ = 0;
    count_ = 0;
    ensureFile_();
    writeHeader_();
}

}
