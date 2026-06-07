#include "AnalyticsUploader.hpp"
#include "CoordinatorConfig.hpp"
#include <array>
#include <cstdio>

namespace gh::app {

using gh::domain::ErrorCode;
using gh::domain::TelemetryRecord;
using Cfg = gh::coord::CoordinatorConfig;

AnalyticsUploader::AnalyticsUploader(gh::domain::ITelemetryQueue& queue,
                                     gh::domain::IHttpClient&     http,
                                     gh::domain::IClock&          clock,
                                     gh::domain::ILogger&         log,
                                     AnalyticsUploaderConfig      cfg) noexcept
    : queue_{queue}, http_{http}, clock_{clock}, log_{log}, cfg_{cfg} {}

void AnalyticsUploader::onReading(const TelemetryRecord& r) noexcept {
    if (queue_.append(r) != ErrorCode::Ok) {
        log_.warn("analytics", "queue append failed");
    }
}

void AnalyticsUploader::tick() noexcept {
    const uint64_t now = clock_.nowMs();
    const uint64_t next_allowed =
        last_flush_ms_ + (backoff_ms_ ? backoff_ms_ : cfg_.flush_period_ms);
    if (now < next_allowed) return;
    if (queue_.size() == 0) {
        last_flush_ms_ = now;
        return;
    }
    doFlush_();
}

void AnalyticsUploader::flushNow() noexcept {
    if (queue_.size() == 0) return;
    doFlush_();
}

void AnalyticsUploader::doFlush_() noexcept {
    static std::array<TelemetryRecord, Cfg::kAnalyticsBatchMaxRecords> scratch;
    const size_t n = queue_.peek(scratch.data(), scratch.size());

    static std::array<char, Cfg::kAnalyticsBuildBufBytes> body;
    const size_t body_len = buildBody_(scratch.data(), n, body.data(), body.size());

    const auto resp = http_.postJson(cfg_.backend_url, cfg_.api_key,
                                     body.data(), body_len,
                                     Cfg::kAnalyticsHttpTimeoutMs);
    last_flush_ms_ = clock_.nowMs();

    const uint32_t start_ms = Cfg::kAnalyticsBackoffStartS * 1000;
    const uint32_t cap_ms   = Cfg::kAnalyticsBackoffCapS * 1000;

    if (resp.error != ErrorCode::Ok || resp.http_status < 0) {
        backoff_ms_ = (backoff_ms_ == 0)
                          ? start_ms
                          : ((backoff_ms_ * 2 <= cap_ms) ? backoff_ms_ * 2 : cap_ms);
        log_.warn("analytics", "transport failure - backoff engaged");
        return;
    }

    if (resp.http_status >= 200 && resp.http_status < 300) {
        queue_.drop(n);
        backoff_ms_ = 0;
        ++batch_seq_;
        log_.info("analytics", "batch accepted");
        return;
    }

    if (resp.http_status >= 400 && resp.http_status < 500) {
        ++poison_4xx_;
        queue_.drop(n);
        backoff_ms_ = 0;
        log_.error("analytics", "4xx - dropping batch");
        return;
    }

    // 5xx
    const uint32_t hinted_ms = static_cast<uint32_t>(resp.retry_after_s) * 1000;
    const uint32_t doubled   = (backoff_ms_ == 0)
                                   ? start_ms
                                   : ((backoff_ms_ * 2 <= cap_ms) ? backoff_ms_ * 2 : cap_ms);
    const uint32_t next      = (hinted_ms > doubled) ? hinted_ms : doubled;
    backoff_ms_ = (next <= cap_ms) ? next : cap_ms;
    log_.warn("analytics", "5xx - backoff engaged");
}

size_t AnalyticsUploader::buildBody_(const TelemetryRecord* records, size_t count,
                                     char* out, size_t cap) noexcept {
    size_t pos = 0;
    auto append = [&](const char* fmt, auto... args) {
        if (pos >= cap) return;
        int wrote = std::snprintf(out + pos, cap - pos, fmt, args...);
        if (wrote > 0) pos += static_cast<size_t>(wrote);
    };
    append("{\"device_id\":\"%s\",\"fw_version\":\"%s\",\"batch_id\":\"b-%u\",\"readings\":[",
           cfg_.device_id, cfg_.fw_version, static_cast<unsigned>(batch_seq_));
    for (size_t i = 0; i < count; ++i) {
        const auto& r = records[i];
        if (i > 0) append("%s", ",");
        if (r.raw == gh::domain::kTelemetryRawNotApplicable) {
            append("{\"ts\":%llu,\"channel_id\":%u,\"kind\":\"%s\",\"value\":%g,\"status\":%u}",
                   static_cast<unsigned long long>(r.ts_unix_ms),
                   static_cast<unsigned>(r.channel_id),
                   gh::domain::telemetryKindWire(r.kind),
                   static_cast<double>(r.value),
                   static_cast<unsigned>(r.status));
        } else {
            append("{\"ts\":%llu,\"channel_id\":%u,\"kind\":\"%s\",\"value\":%g,\"raw\":%d,\"status\":%u}",
                   static_cast<unsigned long long>(r.ts_unix_ms),
                   static_cast<unsigned>(r.channel_id),
                   gh::domain::telemetryKindWire(r.kind),
                   static_cast<double>(r.value),
                   static_cast<int>(r.raw),
                   static_cast<unsigned>(r.status));
        }
    }
    append("%s", "],\"events\":[]}");
    return pos;
}

}
