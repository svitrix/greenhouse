#ifdef ARDUINO

#include "RestStatusRoutes.hpp"
#include <ArduinoJson.h>

namespace gh::presentation {

void RestStatusRoutes::registerOn(AsyncWebServer& server) noexcept {
    server.on("/api/status", HTTP_GET,
        [this](AsyncWebServerRequest* req) {
            gh::domain::SystemInfo info{};
            sysinfo_.snapshot(info);

            uint8_t online = 0;
            uint8_t total  = 0;
            for (const auto& snap : reg_.snapshotAll()) {
                ++total;
                if (snap.online) ++online;
            }

            auto* resp = req->beginResponseStream("application/json");
            resp->addHeader("Cache-Control", "no-store");
            JsonDocument doc;
            doc["device_id"]         = device_id_;
            doc["name"]              = device_id_;
            doc["uptime_s"]          = info.uptime_s;
            doc["firmware_version"]  = info.firmware_version;
            doc["ip"]                = info.ip;
            doc["wifi_rssi_dbm"]     = info.wifi_rssi_dbm;
            doc["mqtt_connected"]    = mqtt_.isConnected();
            doc["node_count_online"] = online;
            doc["node_count_total"]  = total;
            serializeJson(doc, *resp);
            req->send(resp);
        });
}

}  // namespace gh::presentation

#endif  // ARDUINO
