#pragma once
#include <WiFiClientSecure.h>
#include "ports/IHttpClient.hpp"

namespace gh::infra {

// IHttpClient adapter built on Arduino-ESP32 HTTPClient + WiFiClientSecure.
//
// WiFiClientSecure is held as a member — constructed once in the ctor and
// reused for every POST. This pre-pays the ~50 KB TLS heap during init,
// satisfying the global "no malloc after init phase" rule (see spec §3.5).
// HTTPClient itself is stack-scoped per call (small).
//
// TLS:
// - If ca_cert_pem is non-null, pins to that root CA.
// - Otherwise calls setInsecure() — only use for local dev (spec §4.6).
class EspHttpsClient final : public gh::domain::IHttpClient {
public:
    explicit EspHttpsClient(const char* ca_cert_pem = nullptr) noexcept;

    [[nodiscard]] gh::domain::HttpResponse
    postJson(const char* url,
             const char* api_key,
             const char* body,
             size_t      body_len,
             uint32_t    timeout_ms) noexcept override;

    // Same as postJson, but copies the response body (NUL-terminated)
    // into body_out. Used by EspPairingClient to parse the api_key out
    // of the /api/pairing/claim response. body_out is always written
    // (empty string on transport failure).
    [[nodiscard]] gh::domain::HttpResponse
    postJsonWithBody(const char* url,
                     const char* api_key,
                     const char* body,
                     size_t      body_len,
                     uint32_t    timeout_ms,
                     char*       body_out,
                     size_t      body_out_size) noexcept;

private:
    WiFiClientSecure secure_;
};

}
