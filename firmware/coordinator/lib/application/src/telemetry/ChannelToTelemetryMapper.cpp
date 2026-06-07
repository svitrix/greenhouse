#include "ChannelToTelemetryMapper.hpp"
#include "ChannelAttrTable.hpp"
#include "entities/SensorKind.hpp"

namespace gh::app {

namespace {

std::optional<gh::domain::TelemetryKind>
mapKind(gh::domain::SensorKind kind, gh::protocol::Quantity q) noexcept {
    using gh::domain::SensorKind;
    using gh::domain::TelemetryKind;
    using gh::protocol::Quantity;
    if (kind == SensorKind::Air     && q == Quantity::AirTempC)        return TelemetryKind::AirTemp;
    if (kind == SensorKind::Air     && q == Quantity::AirHumidityPct)  return TelemetryKind::AirHumidity;
    if (kind == SensorKind::Soil    && q == Quantity::SoilMoisturePct) return TelemetryKind::SoilMoist;
    if (kind == SensorKind::Soil    && q == Quantity::SoilTempC)       return TelemetryKind::SoilTemp;
    if (kind == SensorKind::Battery && q == Quantity::BatteryPct)      return TelemetryKind::BatteryPct;
    if (kind == SensorKind::Battery && q == Quantity::BatteryVoltageV) return TelemetryKind::BatteryV;
    return std::nullopt;
}

uint8_t channelIdFor(gh::protocol::Quantity q) noexcept {
    for (size_t i = 0; i < gh::protocol::kChannelAttrTableSize; ++i) {
        if (gh::protocol::kChannelAttrTable[i].quantity == q) {
            return gh::protocol::kChannelAttrTable[i].channel_id;
        }
    }
    return 0;
}

}  // namespace

std::optional<gh::domain::TelemetryRecord>
ChannelToTelemetryMapper::map(
    gh::domain::NodeId /*node*/,
    const gh::domain::ChannelSample& s,
    uint64_t unix_ts_ms) noexcept
{
    auto tk = mapKind(s.kind, s.quantity);
    if (!tk) return std::nullopt;

    gh::domain::TelemetryRecord r{};
    r.ts_unix_ms = unix_ts_ms;
    r.channel_id = channelIdFor(s.quantity);
    r.kind       = *tk;
    r.value      = s.value_si;
    r.raw        = gh::domain::kTelemetryRawNotApplicable;
    r.status     = 0;
    return r;
}

}
