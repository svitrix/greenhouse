#pragma once
#include <deque>
#include "ports/ITelemetryQueue.hpp"

namespace gh::test {

class FakeTelemetryQueue final : public gh::domain::ITelemetryQueue {
public:
    explicit FakeTelemetryQueue(size_t cap = 3200) : cap_{cap} {}

    gh::domain::ErrorCode append(const gh::domain::TelemetryRecord& r) noexcept override {
        if (records.size() >= cap_) {
            records.pop_front();
            ++dropped_oldest;
        }
        records.push_back(r);
        return gh::domain::ErrorCode::Ok;
    }
    size_t size() const noexcept override { return records.size(); }
    size_t peek(gh::domain::TelemetryRecord* out, size_t max) const noexcept override {
        size_t n = (max < records.size()) ? max : records.size();
        for (size_t i = 0; i < n; ++i) out[i] = records[i];
        return n;
    }
    void drop(size_t count) noexcept override {
        size_t n = (count < records.size()) ? count : records.size();
        for (size_t i = 0; i < n; ++i) records.pop_front();
    }
    void clear() noexcept override { records.clear(); }

    std::deque<gh::domain::TelemetryRecord> records;
    size_t dropped_oldest = 0;
    size_t cap_;
};

}
