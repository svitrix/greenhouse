#ifdef ARDUINO

#include "RestConfigRoutes.hpp"
#include "entities/AutoWaterConfig.hpp"
#include "entities/MqttCreds.hpp"
#include "entities/WifiCreds.hpp"
#include <ArduinoJson.h>
#include <cstring>

namespace gh::presentation {

namespace {

void writeAutoWaterJson(JsonObject root, const gh::domain::AutoWaterConfig& aw) {
    root["enabled"]            = aw.enabled;
    root["trigger_below_pct"]  = aw.trigger_below_pct;
    root["min_interval_min"]   = aw.min_interval_min;
    root["duration_s"]         = aw.duration_s;
    root["min_fresh_sources"]  = aw.min_fresh_sources;
    root["stale_threshold_s"]  = aw.stale_threshold_s;
}

void writeMqttJson(JsonObject root, const gh::domain::MqttCreds& mc) {
    root["host"]         = mc.host;
    root["port"]         = mc.port;
    root["user"]         = mc.user;
    root["password_set"] = mc.password[0] != '\0';
}

void writeWifiJson(JsonObject root, const gh::domain::WifiCreds& wc) {
    root["ssid"] = wc.ssid;
}

}  // namespace

void RestConfigRoutes::registerOn(AsyncWebServer& server) noexcept {
    server.on("/api/config", HTTP_GET,
        [this](AsyncWebServerRequest* req) {
            auto aw_r = auto_water_store_.load();
            auto mc_r = mqtt_store_.load();
            auto wc_r = wifi_store_.load();

            const gh::domain::AutoWaterConfig aw =
                aw_r.ok() ? aw_r.value : gh::domain::kDefaultAutoWaterConfig;
            const gh::domain::MqttCreds mc =
                mc_r.ok() ? mc_r.value : gh::domain::MqttCreds{};
            const gh::domain::WifiCreds wc =
                wc_r.ok() ? wc_r.value : gh::domain::WifiCreds{};

            auto* resp = req->beginResponseStream("application/json");
            JsonDocument doc;
            writeAutoWaterJson(doc["auto_water"].to<JsonObject>(), aw);
            writeMqttJson    (doc["mqtt"].to<JsonObject>(),       mc);
            writeWifiJson    (doc["wifi"].to<JsonObject>(),       wc);
            serializeJson(doc, *resp);
            req->send(resp);
        }).addMiddleware(&auth_);

    server.on("/api/config", HTTP_POST,
        [this](AsyncWebServerRequest*) {},
        nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
                size_t, size_t) {
            JsonDocument body;
            if (deserializeJson(body, reinterpret_cast<const char*>(data), len)) {
                req->send(400, "application/json", "{\"error\":\"bad_json\"}");
                return;
            }

            if (body["auto_water"].is<JsonObject>()) {
                const auto aw_j = body["auto_water"].as<JsonObject>();
                auto cur = auto_water_store_.load();
                gh::domain::AutoWaterConfig aw =
                    cur.ok() ? cur.value : gh::domain::kDefaultAutoWaterConfig;
                if (aw_j["enabled"].is<bool>()) {
                    aw.enabled = aw_j["enabled"].as<bool>();
                }
                if (aw_j["trigger_below_pct"].is<int>()) {
                    aw.trigger_below_pct =
                        static_cast<uint8_t>(aw_j["trigger_below_pct"].as<int>());
                }
                if (aw_j["min_interval_min"].is<int>()) {
                    aw.min_interval_min =
                        static_cast<uint16_t>(aw_j["min_interval_min"].as<int>());
                }
                if (aw_j["duration_s"].is<int>()) {
                    aw.duration_s =
                        static_cast<uint8_t>(aw_j["duration_s"].as<int>());
                }
                if (aw_j["min_fresh_sources"].is<int>()) {
                    aw.min_fresh_sources =
                        static_cast<uint8_t>(aw_j["min_fresh_sources"].as<int>());
                }
                if (aw_j["stale_threshold_s"].is<int>()) {
                    aw.stale_threshold_s =
                        static_cast<uint32_t>(aw_j["stale_threshold_s"].as<int>());
                }
                if (!aw.valid()) {
                    req->send(400, "application/json",
                              "{\"error\":\"bad_auto_water\"}");
                    return;
                }
                if (auto_water_store_.save(aw) != gh::domain::ErrorCode::Ok) {
                    req->send(500, "application/json",
                              "{\"error\":\"nvs_error\"}");
                    return;
                }
            }

            if (body["mqtt"].is<JsonObject>()) {
                const auto mq_j = body["mqtt"].as<JsonObject>();
                auto cur = mqtt_store_.load();
                gh::domain::MqttCreds mc =
                    cur.ok() ? cur.value : gh::domain::MqttCreds{};
                if (mq_j["host"].is<const char*>()) {
                    std::strncpy(mc.host, mq_j["host"].as<const char*>(),
                                 sizeof(mc.host) - 1);
                    mc.host[sizeof(mc.host) - 1] = '\0';
                }
                if (mq_j["port"].is<int>()) {
                    mc.port = static_cast<uint16_t>(mq_j["port"].as<int>());
                }
                if (mq_j["user"].is<const char*>()) {
                    std::strncpy(mc.user, mq_j["user"].as<const char*>(),
                                 sizeof(mc.user) - 1);
                    mc.user[sizeof(mc.user) - 1] = '\0';
                }
                if (mq_j["password"].is<const char*>()) {
                    const char* p = mq_j["password"].as<const char*>();
                    if (std::strlen(p) > 0) {
                        std::strncpy(mc.password, p, sizeof(mc.password) - 1);
                        mc.password[sizeof(mc.password) - 1] = '\0';
                    }
                }
                if (!mc.valid()) {
                    req->send(400, "application/json",
                              "{\"error\":\"bad_mqtt\"}");
                    return;
                }
                if (mqtt_store_.save(mc) != gh::domain::ErrorCode::Ok) {
                    req->send(500, "application/json",
                              "{\"error\":\"nvs_error\"}");
                    return;
                }
            }

            auto* resp = req->beginResponseStream("application/json");
            JsonDocument out;
            out["ok"]               = true;
            out["restart_required"] = false;
            serializeJson(out, *resp);
            req->send(resp);
        }).addMiddleware(&auth_);
}

}  // namespace gh::presentation

#endif  // ARDUINO
