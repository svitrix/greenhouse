#include "ZigbeeReportRouter.hpp"
#include "AnalyticsUploader.hpp"
#include "telemetry/ChannelToTelemetryMapper.hpp"
#include "network/ZigbeeBindingTable.hpp"

namespace gh::app {

ZigbeeReportRouter::ZigbeeReportRouter(
    gh::domain::INodeRegistry& reg, gh::domain::INodeHistoryStore& hist,
    gh::infra::ZigbeeBindingTable& bind, gh::domain::ILogger& log) noexcept
    : reg_{reg}, hist_{hist}, bind_{bind}, log_{log} {}

void ZigbeeReportRouter::setAnalyticsBridge(
    AnalyticsUploader* uploader, gh::domain::IClock* wall_clock_ms) noexcept
{
    uploader_   = uploader;
    wall_clock_ = wall_clock_ms;
}

void ZigbeeReportRouter::onDeviceAnnounced(
    uint16_t short_addr, uint64_t ieee) noexcept
{
    bind_.onDeviceAnnounced(short_addr, ieee);
    (void)reg_.recordPresence(gh::domain::NodeId{ieee}, short_addr,
                              /*mask*/ 0, /*proto*/ 0);
}

void ZigbeeReportRouter::onDeviceLeft(uint16_t short_addr) noexcept {
    if (auto id = bind_.resolve(short_addr)) {
        reg_.markOffline(*id);
    }
    bind_.onDeviceLeft(short_addr);
}

void ZigbeeReportRouter::onPresenceFrame(
    uint16_t short_addr, uint32_t mask,
    uint16_t proto_version, int8_t rssi) noexcept
{
    auto id = bind_.resolve(short_addr);
    if (!id) {
        log_.warn("zb_router", "presence for unknown short_addr");
        return;
    }
    if (mask != 0 || proto_version != 0) {
        (void)reg_.recordPresence(*id, short_addr, mask, proto_version);
    }
    reg_.recordRssi(*id, rssi);
}

void ZigbeeReportRouter::onChannelSample(
    uint16_t short_addr, gh::domain::ChannelSample s, int8_t rssi) noexcept
{
    auto id = bind_.resolve(short_addr);
    if (!id) {
        log_.warn("zb_router", "sample for unknown short_addr");
        return;
    }
    reg_.recordSample(*id, s);
    reg_.recordRssi  (*id, rssi);
    hist_.recordPoint(*id, s.kind, s.quantity,
        gh::domain::INodeHistoryStore::Point{s.monotonic_ms, s.value_si});

    if (uploader_ != nullptr && wall_clock_ != nullptr) {
        const uint64_t unix_ms = wall_clock_->unixMs();
        if (unix_ms != 0) {
            if (auto rec = ChannelToTelemetryMapper::map(*id, s, unix_ms)) {
                uploader_->onReading(*rec);
            }
        }
    }
}

}
