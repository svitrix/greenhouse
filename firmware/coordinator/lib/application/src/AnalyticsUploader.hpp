#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "CoordinatorConfig.hpp"
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

    // Tests / operator manual trigger. Honours the backoff gate (so a manual
    // trigger cannot hammer a hub that asked us to back off) but bypasses the
    // periodic flush-period gate.
    void flushNow() noexcept;

    [[nodiscard]] uint32_t poison4xxCount() const noexcept { return poison_4xx_; }

private:
    void   doFlush_() noexcept;
    // Serialises up to `count` records into `out`. Returns the byte length on
    // success, or 0 when the batch does not fit `cap` (truncation) — the caller
    // must NOT post a 0-length body (it would drop real telemetry on a hub 400).
    size_t buildBody_(const gh::domain::TelemetryRecord* records,
                      size_t count, char* out, size_t cap) noexcept;

    // C4: backoff gate, wrap-safe in uint32_t millis space.
    [[nodiscard]] bool backoffElapsed_(uint32_t now_ms) const noexcept;

    using Cfg = gh::coord::CoordinatorConfig;

    gh::domain::ITelemetryQueue& queue_;
    gh::domain::IHttpClient&     http_;
    gh::domain::IClock&          clock_;
    gh::domain::ILogger&         log_;
    AnalyticsUploaderConfig      cfg_;
    uint32_t                     last_flush_ms_ = 0;  // millis() space (wraps ~49.7 d)
    uint32_t                     backoff_ms_    = 0;  // 0 = no backoff active
    uint32_t                     poison_4xx_    = 0;
    uint32_t                     batch_seq_     = 0;
    bool                         in_flush_      = false;  // C6 reentrancy guard

    // C6: scratch/body buffers are class members (allocated once at
    // construction), NOT function-static (reentrancy hazard) and NOT stack
    // locals (kAnalyticsBuildBufBytes = 32 KB blows a task stack). Only
    // analytics_task drives doFlush_() in production; in_flush_ documents and
    // guards that single-task invariant.
    std::array<gh::domain::TelemetryRecord, Cfg::kAnalyticsBatchMaxRecords> scratch_{};
    std::array<char, Cfg::kAnalyticsBuildBufBytes>                          body_{};
};

}
