#include "EspPairingClient.hpp"
#include <ArduinoJson.h>
#include <cstdio>
#include <cstring>

namespace gh::infra {

using gh::domain::ErrorCode;

EspPairingClient::EspPairingClient(EspHttpsClient& http) noexcept : http_{http} {}

ErrorCode EspPairingClient::claim(
    const char* backend_url,
    const char* claim_code,
    const char* device_id,
    const char* mac,
    const char* fw_version,
    const char* profile_id,
    char*       api_key_out,
    size_t      api_key_out_size) noexcept
{
    if (api_key_out == nullptr || api_key_out_size < 65) {
        return ErrorCode::InvalidArgument;
    }
    api_key_out[0] = '\0';

    // Build the claim URL. The captive-portal field carries the analytics
    // /ingest URL; replace the "/ingest" suffix with the claim path.
    char url[192];
    std::snprintf(url, sizeof(url), "%s", backend_url);
    char* ingest = std::strstr(url, "/ingest");
    if (ingest != nullptr) {
        std::snprintf(ingest, sizeof(url) - (ingest - url),
                      "/api/pairing/claim");
    } else {
        std::strncat(url, "/api/pairing/claim",
                     sizeof(url) - std::strlen(url) - 1);
    }

    char body[384];
    const int n = std::snprintf(
        body, sizeof(body),
        "{\"claim_code\":\"%s\",\"device_id\":\"%s\",\"mac\":\"%s\","
        "\"fw_version\":\"%s\",\"profile_id\":\"%s\"}",
        claim_code, device_id, mac, fw_version, profile_id
    );
    if (n < 0 || static_cast<size_t>(n) >= sizeof(body)) {
        return ErrorCode::InvalidArgument;
    }

    char resp_body[256];
    const auto resp = http_.postJsonWithBody(
        url, /*api_key=*/"", body, static_cast<size_t>(n),
        /*timeout_ms=*/10'000, resp_body, sizeof(resp_body)
    );

    if (resp.error != ErrorCode::Ok) return resp.error;
    switch (resp.http_status) {
        case 200: break;
        case 410: return ErrorCode::PairingWindowExpired;
        case 409: return ErrorCode::PairingDeviceConflict;
        case 422: return ErrorCode::PairingProfileUnknown;
        default:  return ErrorCode::HttpTransportFailure;
    }

    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, resp_body) != DeserializationError::Ok) {
        return ErrorCode::HttpTransportFailure;
    }
    const char* api_key = doc["api_key"] | static_cast<const char*>(nullptr);
    if (api_key == nullptr) {
        return ErrorCode::HttpTransportFailure;
    }
    std::strncpy(api_key_out, api_key, api_key_out_size - 1);
    api_key_out[api_key_out_size - 1] = '\0';
    return ErrorCode::Ok;
}

}
