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
// TLS policy (default DENY — remediation C1/C2):
// - ca_cert_pem non-null  → pins to that root CA (production path).
// - ca_cert_pem null + allow_insecure_dev=true → setInsecure() and plain-`http://`
//   tolerated. ONLY for local dev (spec §4.6); api_key + telemetry are exposed to
//   MITM on the IoT-VLAN.
// - ca_cert_pem null + allow_insecure_dev=false → every request fails closed with
//   HttpTransportFailure. There is NO silent insecure fallback.
//
// setInsecure()/setCACert() are (re)applied per request BEFORE http.begin(),
// because WiFiClientSecure latches the verification mode at handshake time.
class EspHttpsClient final : public gh::domain::IHttpClient {
public:
    explicit EspHttpsClient(const char* ca_cert_pem        = nullptr,
                            bool        allow_insecure_dev = false) noexcept;

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
    // Validates the URL scheme and applies the per-request TLS verification mode.
    // On success returns true and sets is_https_out. On a policy violation
    // (no CA + not dev-insecure, or http:// without the dev flag) returns false
    // and writes the failure response into fail_out.
    [[nodiscard]] bool prepareTransport_(const char*               url,
                                         bool&                     is_https_out,
                                         gh::domain::HttpResponse& fail_out) noexcept;

    const char* ca_cert_pem_;
    bool        allow_insecure_dev_;
    WiFiClientSecure secure_;
};

}
