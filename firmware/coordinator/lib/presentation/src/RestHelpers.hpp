#pragma once
// Shared inline helpers for all REST route modules. ESP-only (ArduinoJson +
// ESPAsyncWebServer are not available in the native test env).
#ifdef ARDUINO

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <cstddef>
#include <cstring>

namespace gh::presentation::rest {

// Largest request body any /api/* POST/PUT handler will buffer before parsing.
// Bodies are tiny config blobs; anything larger is rejected with 400 up front
// (DoS / heap-blowup guard). §9 mandates fixed-capacity documents.
constexpr std::size_t kMaxBodyBytes = 512;

// Finalise an AsyncJsonResponse built up by a handler: compute payload size,
// stamp the HTTP code + no-store cache header, hand off to the server.
// The server owns and deletes `resp` after sending.
inline void finishJsonResponse(AsyncWebServerRequest* req,
                               AsyncJsonResponse* resp, int code) {
    resp->setLength();
    resp->setCode(code);
    resp->addHeader("Cache-Control", "no-store");
    req->send(resp);
}

// The single error envelope for the whole REST surface: { ok:false, error,
// message }. Every handler routes failures through here so the SPA's zod
// contract sees exactly one shape.
inline void sendError(AsyncWebServerRequest* req,
                      int http, const char* code, const char* message) {
    auto* resp = new AsyncJsonResponse();
    JsonObject root = resp->getRoot().to<JsonObject>();
    root["ok"]      = false;
    root["error"]   = code;
    root["message"] = message;
    finishJsonResponse(req, resp, http);
}

// Reassemble a (possibly chunked) request body into a fixed caller-owned
// buffer. ESPAsyncWebServer may deliver the body across several callbacks with
// (data,len,index,total); parsing each chunk as a whole body truncates input
// and double-sends. Returns true exactly once — when the final chunk has been
// copied and `out` holds the full NUL-terminated body of `out_len` bytes. On an
// over-cap body it sends a 400 and returns false (handler must just return).
[[nodiscard]] inline bool collectBody(AsyncWebServerRequest* req,
                                      const uint8_t* data, std::size_t len,
                                      std::size_t index, std::size_t total,
                                      char* out, std::size_t out_cap,
                                      std::size_t& out_len) noexcept {
    if (total >= out_cap || req->contentLength() >= out_cap) {
        if (index == 0) sendError(req, 400, "body_too_large", "request body too large");
        return false;
    }
    if (index + len <= out_cap) {
        std::memcpy(out + index, data, len);
    }
    if (index + len != total) {
        return false;  // more chunks to come
    }
    out[total] = '\0';
    out_len = total;
    return true;
}

}  // namespace gh::presentation::rest

#endif  // ARDUINO
