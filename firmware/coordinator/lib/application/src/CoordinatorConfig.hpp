#pragma once
#include <cstddef>
#include <cstdint>

namespace gh::coord {

// Coordinator-specific hardware pin assignments.
// Global shared defaults live in shared/application/src/AppConfig.hpp.
struct CoordinatorConfig {
    // GPIO18 → relay CH1 IN → pump.
    // Canonical: docs/hardware/reference/canonical-values.md#coordinator-gpio
    static constexpr uint8_t kRelayIn1Pin = 18;

    // Consecutive failed boot attempts after which WifiProvisioner forces
    // provisioning mode even with valid credentials in NVS. Self-heal
    // mechanism for the "router password changed" scenario.
    static constexpr uint8_t kMaxConsecutiveFailedBoots = 10;

    // HTTP Basic Auth rate limiting on /api/* — caps brute-force attempts
    // at 5 requests per 10 s per client IP. Applied alongside the auth
    // middleware in RestApi::start().
    static constexpr uint16_t kAuthRateLimitMaxRequests = 5;
    static constexpr uint32_t kAuthRateLimitWindowMs    = 10'000;
    // AsyncRateLimitMiddleware::setWindowSize() takes seconds.
    static constexpr uint32_t kAuthRateLimitWindowS     = kAuthRateLimitWindowMs / 1000U;

    // Analytics uploader (spec 2026-06-01 §3.3, §3.4).
    static constexpr uint32_t kAnalyticsFlushPeriodSDefault = 900;        // 15 min
    static constexpr uint32_t kAnalyticsFlushPeriodSMin     = 60;
    static constexpr uint32_t kAnalyticsFlushPeriodSMax     = 3600;
    static constexpr uint32_t kAnalyticsBackoffStartS       = 60;
    static constexpr uint32_t kAnalyticsBackoffCapS         = 3600;
    static constexpr uint32_t kAnalyticsHttpTimeoutMs       = 10'000;
    static constexpr size_t   kAnalyticsBatchMaxRecords     = 500;
    static constexpr size_t   kAnalyticsBuildBufBytes       = 32 * 1024;  // stack NDJSON buf
    static constexpr uint32_t kAnalyticsTaskStackBytes      = 8 * 1024;
    static constexpr uint32_t kAnalyticsTaskPriority        = 3;          // below MQTT

    // Combined task that drives IrrigationService::tick(), NodePruneService::tick(),
    // TelemetryPublisher::tick() and HomeAssistantDiscoveryService::reconcile()
    // at 0.2 Hz. Stack sized at 4 KB with ≥20 % headroom against the largest call
    // chain (TelemetryPublisher stack-only JSON formatting).
    static constexpr uint32_t kCoordinatorTaskStackBytes    = 4 * 1024;
    static constexpr uint32_t kCoordinatorTaskPriority      = 1;

    // On-board WS2812 status LED (DevKitM-1: GPIO8, board-driven RGB).
    // Canonical: docs/hardware/reference/canonical-values.md#coordinator-gpio
    static constexpr uint8_t  kStatusLedGpio            = 8;
    static constexpr uint8_t  kStatusLedBrightnessPct   = 50;   // WS2812 is blinding at full
    static constexpr uint32_t kLedTaskStackBytes        = 2 * 1024;
    static constexpr uint32_t kLedTaskPriority          = 1;
    static constexpr uint32_t kLedTickMs               = 100;   // blink resolution
};

}  // namespace gh::coord
