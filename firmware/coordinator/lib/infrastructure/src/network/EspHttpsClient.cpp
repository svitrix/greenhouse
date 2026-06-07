#include "EspHttpsClient.hpp"
#include <Arduino.h>
#include <HTTPClient.h>
#include <cstdio>
#include <cstring>

namespace gh::infra {

using gh::domain::ErrorCode;
using gh::domain::HttpResponse;

EspHttpsClient::EspHttpsClient(const char* ca_cert_pem) noexcept {
    if (ca_cert_pem) {
        secure_.setCACert(ca_cert_pem);
    } else {
        secure_.setInsecure();
    }
}

HttpResponse EspHttpsClient::postJson(const char* url,
                                      const char* api_key,
                                      const char* body,
                                      size_t      body_len,
                                      uint32_t    timeout_ms) noexcept {
    HTTPClient http;
    http.setTimeout(timeout_ms);

    const bool is_https = (std::strncmp(url, "https://", 8) == 0);
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

    HTTPClient http;
    http.setTimeout(timeout_ms);

    const bool is_https = (std::strncmp(url, "https://", 8) == 0);
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

    if (body_out != nullptr && body_out_size > 0) {
        const String s = http.getString();
        std::strncpy(body_out, s.c_str(), body_out_size - 1);
        body_out[body_out_size - 1] = '\0';
    }
    http.end();

    if (code < 0) {
        return HttpResponse{-1, 0, ErrorCode::HttpTransportFailure};
    }
    return HttpResponse{static_cast<int16_t>(code), 0, ErrorCode::Ok};
}

}
