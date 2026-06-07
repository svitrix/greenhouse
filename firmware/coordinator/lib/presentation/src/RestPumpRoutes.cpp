#ifdef ARDUINO

#include "RestPumpRoutes.hpp"
#include "irrigation/AutoWaterDecision.hpp"
#include "entities/PumpState.hpp"
#include <ArduinoJson.h>
#include <cstring>

namespace gh::presentation {

namespace {

const char* pumpStateCode(gh::domain::PumpState s) noexcept {
    switch (s) {
        case gh::domain::PumpState::Off:          return "OFF";
        case gh::domain::PumpState::On:           return "ON";
        case gh::domain::PumpState::SafetyLocked: return "LOCKED";
    }
    return "OFF";
}

}  // namespace

void RestPumpRoutes::registerOn(AsyncWebServer& server) noexcept {
    server.on("/api/pump", HTTP_GET,
        [this](AsyncWebServerRequest* req) {
            auto* resp = req->beginResponseStream("application/json");
            JsonDocument doc;
            doc["state"]       = pumpStateCode(pump_.state());
            doc["remaining_s"] = 0;
            doc["last_run_ms"] = 0;
            serializeJson(doc, *resp);
            req->send(resp);
        }).addMiddleware(&auth_);

    server.on("/api/pump", HTTP_POST,
        [this](AsyncWebServerRequest*) {},
        nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
                size_t, size_t) {
            JsonDocument body;
            if (deserializeJson(body, reinterpret_cast<const char*>(data), len)) {
                req->send(400, "application/json", "{\"error\":\"bad_json\"}");
                return;
            }
            const char* st = body["state"] | "";
            gh::app::AutoWaterDecision d{};
            if (std::strcmp(st, "ON") == 0) {
                d = irrigation_.requestOn();
            } else if (std::strcmp(st, "OFF") == 0) {
                d = irrigation_.requestOff();
            } else {
                req->send(400, "application/json", "{\"error\":\"bad_state\"}");
                return;
            }
            JsonDocument out;
            out["state"]   = pumpStateCode(pump_.state());
            out["outcome"] = gh::app::outcomeCode(d.outcome);
            if (d.outcome == gh::app::AutoWaterOutcome::Started) {
                out["ok"] = true;
                auto* resp = req->beginResponseStream("application/json");
                serializeJson(out, *resp);
                req->send(resp);
            } else {
                out["ok"]             = false;
                out["lockout_reason"] = gh::app::outcomeCode(d.outcome);
                String body_str;
                serializeJson(out, body_str);
                req->send(409, "application/json", body_str);
            }
        }).addMiddleware(&auth_);
}

}  // namespace gh::presentation

#endif  // ARDUINO
