#include "AnalyticsUploader.hpp"
#include <cmath>
#include <cstdio>

namespace gh::app {

using gh::domain::ErrorCode;
using gh::domain::TelemetryRecord;

// a single worst-case record (long-long ts + raw + headroom) must fit so
// buildBody_ can serialise at least one record. ~160 B is generous; the build
// buffer is 32 KB, so the envelope + many records always fit.
static_assert(gh::coord::CoordinatorConfig::kAnalyticsBuildBufBytes >= 256,
              "analytics build buffer too small for one record + envelope");

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

bool AnalyticsUploader::backoffElapsed_(uint32_t now_ms) const noexcept {
    const uint32_t period = backoff_ms_ ? backoff_ms_ : cfg_.flush_period_ms;
    // wrap-safe comparison in uint32_t millis() space — see MqttClient.cpp:45.
    // The signed difference stays correct across the ~49.7-day rollover.
    const uint32_t next_allowed = last_flush_ms_ + period;
    return static_cast<int32_t>(now_ms - next_allowed) >= 0;
}

void AnalyticsUploader::tick() noexcept {
    const uint32_t now = clock_.nowMs();
    if (!backoffElapsed_(now)) return;
    if (queue_.size() == 0) {
        last_flush_ms_ = now;
        return;
    }
    doFlush_();
}

void AnalyticsUploader::flushNow() noexcept {
    // honour an active backoff so a manual/test trigger cannot hammer a hub
    // that asked us to back off. The periodic flush-period gate is still bypassed.
    if (backoff_ms_ != 0 && !backoffElapsed_(clock_.nowMs())) return;
    if (queue_.size() == 0) return;
    doFlush_();
}

void AnalyticsUploader::doFlush_() noexcept {
    if (in_flush_) {
        // single-task invariant violated (production: only analytics_task
        // calls this). Refuse to corrupt a build already in progress.
        log_.warn("analytics", "doFlush reentered - skipped");
        return;
    }
    in_flush_ = true;

    size_t n = queue_.peek(scratch_.data(), scratch_.size());

    // serialise the batch; on overflow buildBody_ returns 0. Shrink the
    // batch (halving) until it fits the buffer rather than stalling forever on
    // an over-large head. A single record that still does not fit is dropped
    // (poison) so the queue can make progress.
    size_t body_len = buildBody_(scratch_.data(), n, body_.data(), body_.size());
    while (body_len == 0 && n > 1) {
        n /= 2;
        body_len = buildBody_(scratch_.data(), n, body_.data(), body_.size());
    }
    if (body_len == 0) {
        last_flush_ms_ = clock_.nowMs();
        if (n == 1) {
            // One record that cannot be serialised into 32 KB is malformed —
            // drop it so it does not wedge the queue head forever.
            ++poison_4xx_;
            queue_.drop(1);
            log_.error("analytics", "single record too large - dropped");
        } else {
            log_.error("analytics", "empty batch did not build - skipped");
        }
        in_flush_ = false;
        return;
    }

    const auto resp = http_.postJson(cfg_.backend_url, cfg_.api_key,
                                     body_.data(), body_len,
                                     Cfg::kAnalyticsHttpTimeoutMs);
    last_flush_ms_ = clock_.nowMs();
    // Scratch/body buffers are no longer touched below — release the guard
    // before the (early-returning) status-handling block.
    in_flush_ = false;

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
    size_t pos       = 0;
    bool   truncated = false;
    // detect overflow. snprintf returns the length it WOULD have written;
    // if that exceeds the remaining space, the JSON is incomplete → fail closed
    // (return 0) so doFlush_ skips the post instead of shipping a broken body.
    auto append = [&](const char* fmt, auto... args) {
        if (truncated) return;
        const int wrote = std::snprintf(out + pos, cap - pos, fmt, args...);
        if (wrote < 0 || static_cast<size_t>(wrote) >= cap - pos) {
            truncated = true;
            return;
        }
        pos += static_cast<size_t>(wrote);
    };
    append("{\"device_id\":\"%s\",\"fw_version\":\"%s\",\"batch_id\":\"b-%u\",\"readings\":[",
           cfg_.device_id, cfg_.fw_version, static_cast<unsigned>(batch_seq_));
    for (size_t i = 0; i < count; ++i) {
        const auto& r = records[i];
        if (i > 0) append("%s", ",");
        // %g on a non-finite float emits bare `nan`/`inf`, which is invalid
        // JSON. Render the value token separately — `null` for non-finite so the
        // hub records "no value" instead of choking on a malformed body.
        char value_tok[32];
        if (std::isfinite(static_cast<double>(r.value))) {
            std::snprintf(value_tok, sizeof(value_tok), "%g",
                          static_cast<double>(r.value));
        } else {
            std::snprintf(value_tok, sizeof(value_tok), "null");
        }
        if (r.raw == gh::domain::kTelemetryRawNotApplicable) {
            append("{\"ts\":%llu,\"channel_id\":%u,\"kind\":\"%s\",\"value\":%s,\"status\":%u}",
                   static_cast<unsigned long long>(r.ts_unix_ms),
                   static_cast<unsigned>(r.channel_id),
                   gh::domain::telemetryKindWire(r.kind),
                   value_tok,
                   static_cast<unsigned>(r.status));
        } else {
            append("{\"ts\":%llu,\"channel_id\":%u,\"kind\":\"%s\",\"value\":%s,\"raw\":%d,\"status\":%u}",
                   static_cast<unsigned long long>(r.ts_unix_ms),
                   static_cast<unsigned>(r.channel_id),
                   gh::domain::telemetryKindWire(r.kind),
                   value_tok,
                   static_cast<int>(r.raw),
                   static_cast<unsigned>(r.status));
        }
    }
    append("%s", "],\"events\":[]}");
    return truncated ? 0 : pos;
}

}
