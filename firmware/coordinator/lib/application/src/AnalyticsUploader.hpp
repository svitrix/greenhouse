#pragma once
#include <cstdint>
#include "entities/TelemetryRecord.hpp"
#include "ports/IClock.hpp"
#include "ports/IHttpClient.hpp"
#include "ports/ILogger.hpp"
#include "ports/ITelemetryQueue.hpp"

namespace gh::app {

struct AnalyticsUploaderConfig {
    const char* backend_url;
    const char* api_key;
    const char* device_id;
    const char* fw_version;
    uint32_t    flush_period_ms;
};

class AnalyticsUploader {
public:
    AnalyticsUploader(gh::domain::ITelemetryQueue& queue,
                      gh::domain::IHttpClient&     http,
                      gh::domain::IClock&          clock,
                      gh::domain::ILogger&         log,
                      AnalyticsUploaderConfig      cfg) noexcept;

    // Hot path. Called from Zigbee callback context. O(1), no allocation.
    void onReading(const gh::domain::TelemetryRecord& r) noexcept;

    // Periodic. Called from analytics_task. Internally compares now() to
    // last flush + backoff.
    void tick() noexcept;

    // Tests / operator manual trigger. Bypasses the timer / backoff gate.
    void flushNow() noexcept;

    [[nodiscard]] uint32_t poison4xxCount() const noexcept { return poison_4xx_; }

private:
    void doFlush_() noexcept;
    size_t buildBody_(const gh::domain::TelemetryRecord* records,
                      size_t count, char* out, size_t cap) noexcept;

    gh::domain::ITelemetryQueue& queue_;
    gh::domain::IHttpClient&     http_;
    gh::domain::IClock&          clock_;
    gh::domain::ILogger&         log_;
    AnalyticsUploaderConfig      cfg_;
    uint64_t                     last_flush_ms_ = 0;
    uint32_t                     backoff_ms_    = 0;  // 0 = no backoff active
    uint32_t                     poison_4xx_    = 0;
    uint32_t                     batch_seq_     = 0;
};

}
