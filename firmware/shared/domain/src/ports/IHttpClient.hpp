#pragma once
#include <cstddef>
#include <cstdint>
#include "errors/ErrorCode.hpp"

namespace gh::domain {

struct HttpResponse {
    int16_t  http_status;     // -1 on transport failure
    uint16_t retry_after_s;   // 0 if not set
    ErrorCode error;          // Ok on transport success regardless of HTTP status
};

struct IHttpClient {
    virtual ~IHttpClient() = default;

    // POST `body_len` bytes from `body` as JSON to `url` with
    // `Authorization: Bearer <api_key>`. Blocks up to `timeout_ms`.
    // Returns Ok on transport success (caller inspects http_status).
    // Returns HttpTransportFailure / HttpTimeout / TlsHandshakeFailed on
    // transport-level problems.
    [[nodiscard]] virtual HttpResponse
    postJson(const char* url,
             const char* api_key,
             const char* body,
             size_t      body_len,
             uint32_t    timeout_ms) noexcept = 0;
};

}
