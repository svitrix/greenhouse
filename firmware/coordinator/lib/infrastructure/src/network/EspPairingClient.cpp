#include "EspPairingClient.hpp"
#include "PairingUrl.hpp"
#include "util/ClaimCode.hpp"
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

    // Defence in depth: the claim code is also validated server-side in the
    // provisioning handler, but never trust an unvalidated value into the body.
    if (claim_code == nullptr ||
        !gh::domain::isValidClaimCode(claim_code)) {
        return ErrorCode::InvalidArgument;
    }

    // Build the claim URL from the analytics /ingest URL (pure helper).
    char url[192];
    if (!buildClaimUrl(backend_url, url, sizeof(url))) {
        return ErrorCode::InvalidArgument;
    }

    // Build the JSON body with ArduinoJson so any byte in device_id / mac /
    // fw_version / profile_id is correctly escaped (no raw snprintf injection).
    StaticJsonDocument<384> req;
    req["claim_code"]  = claim_code;
    req["device_id"]   = device_id;
    req["mac"]         = mac;
    req["fw_version"]  = fw_version;
    req["profile_id"]  = profile_id;
    char body[384];
    const size_t n = serializeJson(req, body, sizeof(body));
    if (n == 0 || n >= sizeof(body)) {
        return ErrorCode::InvalidArgument;
    }

    char resp_body[256];
    const auto resp = http_.postJsonWithBody(
        url, /*api_key=*/"", body, n,
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
