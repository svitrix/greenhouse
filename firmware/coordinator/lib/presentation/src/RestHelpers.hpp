#pragma once
// Shared inline helpers for all REST route modules. ESP-only (ArduinoJson +
// ESPAsyncWebServer are not available in the native test env).
#ifdef ARDUINO

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>

namespace gh::presentation::rest {

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

inline void sendError(AsyncWebServerRequest* req,
                      int http, const char* code, const char* message) {
    auto* resp = new AsyncJsonResponse();
    JsonObject root = resp->getRoot().to<JsonObject>();
    root["ok"]      = false;
    root["error"]   = code;
    root["message"] = message;
    finishJsonResponse(req, resp, http);
}

}  // namespace gh::presentation::rest

#endif  // ARDUINO
