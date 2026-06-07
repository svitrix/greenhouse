#include "NodeViewBuilder.hpp"
#include "JsonHelpers.hpp"
#include "Quantity.hpp"
#include <cstdio>

namespace gh::presentation {

static const char* unitFor(gh::protocol::Quantity q) noexcept {
    switch (q) {
        case gh::protocol::Quantity::AirTempC:
        case gh::protocol::Quantity::SoilTempC:        return "°C";
        case gh::protocol::Quantity::AirHumidityPct:
        case gh::protocol::Quantity::SoilMoisturePct:
        case gh::protocol::Quantity::BatteryPct:       return "%";
        case gh::protocol::Quantity::BatteryVoltageV:  return "V";
    }
    return "";
}

void NodeViewBuilder::build(const gh::domain::NodeSnapshot& snap,
                             const gh::domain::INodeAliasStore& aliases,
                             uint32_t now_ms, JsonObject out) noexcept
{
    const auto hex = snap.id.toHex16();
    out["ieee"] = hex.data();

    char short_buf[8];
    std::snprintf(short_buf, sizeof(short_buf), "0x%04X", snap.short_addr);
    out["short_addr"] = short_buf;

    auto a = aliases.alias(snap.id);
    if (a.has_value()) out["alias"] = a->data();
    else               out["alias"] = static_cast<const char*>(nullptr);

    out["online"]                 = snap.online;
    out["last_seen_s"]            = (now_ms - snap.last_seen_ms) / 1000u;
    out["rssi_dbm"]               = snap.last_rssi_dbm;
    out["proto_version"]          = snap.proto_version;
    out["proto_version_mismatch"] = snap.proto_version_mismatch;

    char mask_buf[8];
    std::snprintf(mask_buf, sizeof(mask_buf), "0x%02X",
                   static_cast<unsigned>(snap.present_mask & 0xFFu));
    out["present_mask"] = mask_buf;

    JsonArray readings = out["readings"].to<JsonArray>();
    for (const auto& s : snap.samples) {
        JsonObject e = readings.add<JsonObject>();
        e["kind"]     = kindCode(s.kind);
        e["quantity"] = gh::protocol::quantityCode(s.quantity);
        e["value"]    = s.value_si;
        e["unit"]     = unitFor(s.quantity);
        e["age_s"]    = (now_ms - s.monotonic_ms) / 1000u;
    }
}

}
