#include "HomeAssistantDiscoveryService.hpp"
#include "ChannelAttrTable.hpp"
#include "JsonHelpers.hpp"
#include <ArduinoJson.h>
#include <etl/vector.h>
#include <cstdio>
#include <string_view>

namespace gh::presentation {

namespace {

[[nodiscard]] const char* devClassFor(gh::protocol::Quantity q) noexcept {
    switch (q) {
        case gh::protocol::Quantity::AirTempC:        return "temperature";
        case gh::protocol::Quantity::SoilTempC:       return "temperature";
        case gh::protocol::Quantity::AirHumidityPct:  return "humidity";
        case gh::protocol::Quantity::SoilMoisturePct: return "moisture";
        case gh::protocol::Quantity::BatteryPct:      return "battery";
        case gh::protocol::Quantity::BatteryVoltageV: return "voltage";
    }
    return "";
}

[[nodiscard]] const char* unitFor(gh::protocol::Quantity q) noexcept {
    switch (q) {
        case gh::protocol::Quantity::AirTempC:
        case gh::protocol::Quantity::SoilTempC:        return "\xC2\xB0""C";  // UTF-8 °C
        case gh::protocol::Quantity::AirHumidityPct:
        case gh::protocol::Quantity::SoilMoisturePct:
        case gh::protocol::Quantity::BatteryPct:       return "%";
        case gh::protocol::Quantity::BatteryVoltageV:  return "V";
    }
    return "";
}

[[nodiscard]] gh::domain::SensorKind kindForQuantity(gh::protocol::Quantity q) noexcept {
    for (size_t i = 0; i < gh::protocol::kChannelAttrTableSize; ++i) {
        if (gh::protocol::kChannelAttrTable[i].quantity == q) {
            return gh::protocol::kChannelAttrTable[i].kind;
        }
    }
    return gh::domain::SensorKind::Air;
}

}  // namespace

void HomeAssistantDiscoveryService::publishEntity_(
    gh::domain::NodeId id, gh::protocol::Quantity q,
    gh::domain::SensorKind kind) noexcept
{
    const auto hex = id.toHex16();
    const char* qc = gh::protocol::quantityCode(q);
    const char* kc = kindCode(kind);

    char topic[160], uniq[96], stat[96], avty[96], devid[48], dev_name[40], name[80];
    std::snprintf(topic, sizeof(topic),
        "homeassistant/sensor/gh_node_%s_%s_%s/config", hex.data(), kc, qc);
    std::snprintf(uniq, sizeof(uniq), "gh_node_%s_%s_%s", hex.data(), kc, qc);
    std::snprintf(stat, sizeof(stat),
        "greenhouse/%s/nodes/%s/%s", device_id_, hex.data(), qc);
    std::snprintf(avty, sizeof(avty),
        "greenhouse/%s/nodes/%s/online", device_id_, hex.data());
    std::snprintf(devid, sizeof(devid), "gh_node_%s", hex.data());

    auto alias = aliases_.alias(id);
    if (alias.has_value()) {
        std::snprintf(dev_name, sizeof(dev_name), "%s", alias->data());
    } else {
        std::snprintf(dev_name, sizeof(dev_name), "Node 0x%s", hex.data() + 12);
    }
    std::snprintf(name, sizeof(name), "%s %s %s", dev_name, kc, qc);

    JsonDocument doc;
    doc["name"]         = name;
    doc["uniq_id"]      = uniq;
    doc["stat_t"]       = stat;
    doc["avty_t"]       = avty;
    doc["pl_avail"]     = "true";
    doc["pl_not_avail"] = "false";
    doc["unit_of_meas"] = unitFor(q);
    doc["dev_cla"]      = devClassFor(q);
    doc["stat_cla"]     = "measurement";
    JsonObject dev = doc["dev"].to<JsonObject>();
    dev["identifiers"].to<JsonArray>().add(devid);
    dev["name"]         = dev_name;
    dev["manufacturer"] = "diy-greenhouse";
    dev["model"]        = "gh-sensor-node-v2";

    char buf[640];
    const size_t n = serializeJson(doc, buf, sizeof(buf));
    (void)mqtt_.publish(topic, std::string_view(buf, n), /*retain*/ true);
    published_.insert(Key{id, q});
}

void HomeAssistantDiscoveryService::unpublishEntity_(
    gh::domain::NodeId id, gh::protocol::Quantity q) noexcept
{
    const auto hex = id.toHex16();
    const char* qc = gh::protocol::quantityCode(q);
    const char* kc = kindCode(kindForQuantity(q));

    char topic[160];
    std::snprintf(topic, sizeof(topic),
        "homeassistant/sensor/gh_node_%s_%s_%s/config", hex.data(), kc, qc);
    (void)mqtt_.publish(topic, std::string_view{""}, /*retain*/ true);
    published_.erase(Key{id, q});
}

void HomeAssistantDiscoveryService::reconcile() noexcept {
    if (!mqtt_.isConnected()) return;

    etl::flat_set<Key, 64> desired;
    for (const auto& snap : reg_.snapshotAll()) {
        for (size_t i = 0; i < gh::protocol::kChannelAttrTableSize; ++i) {
            const auto& e = gh::protocol::kChannelAttrTable[i];
            if ((snap.present_mask & (1u << e.channel_id)) == 0u) continue;
            desired.insert(Key{snap.id, e.quantity});
        }
    }

    for (const auto& k : desired) {
        if (published_.find(k) != published_.end()) continue;
        publishEntity_(k.id, k.q, kindForQuantity(k.q));
    }
    // Collect keys to unpublish into a temporary buffer; mutating the set
    // during iteration is undefined.
    etl::vector<Key, 64> to_unpublish;
    for (const auto& k : published_) {
        if (desired.find(k) == desired.end()) {
            to_unpublish.push_back(k);
        }
    }
    for (const auto& k : to_unpublish) {
        unpublishEntity_(k.id, k.q);
    }
}

}
