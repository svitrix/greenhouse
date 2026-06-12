#include "EspHttpsClient.hpp"
#include <Arduino.h>
#include <HTTPClient.h>
#include <cstdio>
#include <cstring>

namespace gh::infra {

using gh::domain::ErrorCode;
using gh::domain::HttpResponse;

EspHttpsClient::EspHttpsClient(const char* ca_cert_pem,
                               bool        allow_insecure_dev) noexcept
    : ca_cert_pem_{ca_cert_pem}, allow_insecure_dev_{allow_insecure_dev} {}

bool EspHttpsClient::prepareTransport_(const char*   url,
                                       bool&         is_https_out,
                                       HttpResponse& fail_out) noexcept {
    is_https_out = (std::strncmp(url, "https://", 8) == 0);

    // C2: a plain http:// URL would leak the Bearer api_key + telemetry in
    // cleartext. Refuse it unless the operator opted into dev-insecure mode.
    if (!is_https_out && !allow_insecure_dev_) {
        fail_out = HttpResponse{-1, 0, ErrorCode::HttpTransportFailure};
        return false;
    }

    if (is_https_out) {
        if (ca_cert_pem_ != nullptr) {
            secure_.setCACert(ca_cert_pem_);
        } else if (allow_insecure_dev_) {
            // C1: no pinned CA — accept any cert ONLY in explicit dev mode.
            secure_.setInsecure();
        } else {
            // C1 fail-closed: no CA and not dev-insecure → do NOT connect.
            fail_out = HttpResponse{-1, 0, ErrorCode::TlsHandshakeFailed};
            return false;
        }
    }
    return true;
}

HttpResponse EspHttpsClient::postJson(const char* url,
                                      const char* api_key,
                                      const char* body,
                                      size_t      body_len,
                                      uint32_t    timeout_ms) noexcept {
    bool         is_https = false;
    HttpResponse fail{};
    if (!prepareTransport_(url, is_https, fail)) {
        return fail;
    }

    HTTPClient http;
    http.setTimeout(timeout_ms);

    const bool ok = is_https ? http.begin(secure_, url) : http.begin(url);
    if (!ok) {
        return HttpResponse{-1, 0, ErrorCode::HttpTransportFailure};
    }

    char auth[160];
    std::snprintf(auth, sizeof(auth), "Bearer %s", api_key);
    http.addHeader("Authorization", auth);
    http.addHeader("Content-Type", "application/json");

    const char* retry_hdr[] = {"Retry-After"};
    http.collectHeaders(retry_hdr, 1);

    const int code = http.POST(
        reinterpret_cast<uint8_t*>(const_cast<char*>(body)),
        static_cast<size_t>(body_len)
    );

    uint16_t retry_after = 0;
    {
        const String ra = http.header("Retry-After");
        if (ra.length() > 0) {
            const long parsed = ra.toInt();
            if (parsed > 0 && parsed < 65535) {
                retry_after = static_cast<uint16_t>(parsed);
            }
        }
    }
    http.end();

    if (code < 0) {
        return HttpResponse{-1, 0, ErrorCode::HttpTransportFailure};
    }
    return HttpResponse{static_cast<int16_t>(code), retry_after, ErrorCode::Ok};
}

HttpResponse EspHttpsClient::postJsonWithBody(const char* url,
                                              const char* api_key,
                                              const char* body,
                                              size_t      body_len,
                                              uint32_t    timeout_ms,
                                              char*       body_out,
                                              size_t      body_out_size) noexcept {
    if (body_out != nullptr && body_out_size > 0) body_out[0] = '\0';

    bool         is_https = false;
    HttpResponse fail{};
    if (!prepareTransport_(url, is_https, fail)) {
        return fail;
    }

    HTTPClient http;
    http.setTimeout(timeout_ms);

    const bool ok = is_https ? http.begin(secure_, url) : http.begin(url);
    if (!ok) {
        return HttpResponse{-1, 0, ErrorCode::HttpTransportFailure};
    }

    if (api_key != nullptr && api_key[0] != '\0') {
        char auth[160];
        std::snprintf(auth, sizeof(auth), "Bearer %s", api_key);
        http.addHeader("Authorization", auth);
    }
    http.addHeader("Content-Type", "application/json");

    const int code = http.POST(
        reinterpret_cast<uint8_t*>(const_cast<char*>(body)),
        static_cast<size_t>(body_len)
    );

    // C3: read the response into the caller's fixed buffer via a bounded
    // stream read instead of http.getString() (which heap-allocates an
    // unbounded Arduino String from an attacker/buggy-hub-controlled body).
    // Cap at the smaller of the caller buffer and a known Content-Length so a
    // short body does not stall readBytes() for the whole stream timeout.
    if (body_out != nullptr && body_out_size > 0) {
        WiFiClient* stream = http.getStreamPtr();
        if (stream != nullptr) {
            size_t want = body_out_size - 1;
            const int content_len = http.getSize();  // -1 if chunked / unknown
            if (content_len >= 0 && static_cast<size_t>(content_len) < want) {
                want = static_cast<size_t>(content_len);
            }
            const size_t got =
                stream->readBytes(reinterpret_cast<uint8_t*>(body_out), want);
            body_out[got] = '\0';
        }
    }
    http.end();

    if (code < 0) {
        return HttpResponse{-1, 0, ErrorCode::HttpTransportFailure};
    }
    return HttpResponse{static_cast<int16_t>(code), 0, ErrorCode::Ok};
}

}
