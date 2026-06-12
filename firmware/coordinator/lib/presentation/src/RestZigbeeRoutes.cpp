#ifdef ARDUINO

#include "RestZigbeeRoutes.hpp"
#include "RestHelpers.hpp"
#include <ArduinoJson.h>

namespace gh::presentation {

namespace {
constexpr int kPermitJoinMinS = 1;
constexpr int kPermitJoinMaxS = 254;
}  // namespace

void RestZigbeeRoutes::registerOn(AsyncWebServer& server) noexcept {
    server.on("/api/zigbee/permit-join", HTTP_POST,
        [](AsyncWebServerRequest*) {},
        nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
                size_t index, size_t total) {
            size_t body_len = 0;
            if (!rest::collectBody(req, data, len, index, total,
                                   body_buf_, sizeof(body_buf_), body_len)) {
                return;
            }
            JsonDocument body;
            if (deserializeJson(body, body_buf_, body_len)) {
                rest::sendError(req, 400, "bad_json", "malformed JSON body");
                return;
            }
            if (!body["duration_s"].is<int>()) {
                rest::sendError(req, 400, "missing_duration", "duration_s required");
                return;
            }
            const int dur = body["duration_s"].as<int>();
            if (dur < kPermitJoinMinS || dur > kPermitJoinMaxS) {
                rest::sendError(req, 400, "bad_duration", "duration_s out of range");
                return;
            }
            if (zb_.permitJoin(static_cast<uint8_t>(dur)) !=
                gh::domain::ErrorCode::Ok) {
                rest::sendError(req, 500, "permit_join_failed", "permit-join failed");
                return;
            }
            auto* resp = req->beginResponseStream("application/json");
            resp->addHeader("Cache-Control", "no-store");
            JsonDocument out;
            out["ok"]         = true;
            out["duration_s"] = dur;
            serializeJson(out, *resp);
            req->send(resp);
        });
}

}  // namespace gh::presentation

#endif  // ARDUINO
