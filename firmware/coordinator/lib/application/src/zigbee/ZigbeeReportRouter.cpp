#include "ZigbeeReportRouter.hpp"
#include "AnalyticsUploader.hpp"
#include "telemetry/ChannelToTelemetryMapper.hpp"

namespace gh::app {

ZigbeeReportRouter::ZigbeeReportRouter(
    gh::domain::INodeRegistry& reg, gh::domain::INodeHistoryStore& hist,
    gh::domain::IShortAddrResolver& bind, gh::domain::ILogger& log) noexcept
    : reg_{reg}, hist_{hist}, bind_{bind}, log_{log} {}

void ZigbeeReportRouter::setAnalyticsBridge(
    AnalyticsUploader* uploader, gh::domain::IClock* wall_clock_ms) noexcept
{
    uploader_   = uploader;
    wall_clock_ = wall_clock_ms;
}

std::optional<gh::domain::NodeId>
ZigbeeReportRouter::verifiedNode_(uint16_t short_addr, uint64_t source_ieee,
                                  const char* ctx) noexcept
{
    auto id = bind_.resolve(short_addr);
    if (!id) {
        log_.warn("zb_router", ctx);
        return std::nullopt;
    }
    // Reject reports whose APS source IEEE does not match the IEEE bound to
    // this short_addr (spoofed short_addr → quorum/cloud poisoning). A frame
    // with an unknown source IEEE (0) is treated as unverifiable and dropped.
    if (source_ieee == 0 || source_ieee != id->ieee) {
        log_.warn("zb_router", "report IEEE mismatch — dropped (spoof guard)");
        return std::nullopt;
    }
    return id;
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
    uint16_t short_addr, uint64_t source_ieee, uint32_t mask,
    uint16_t proto_version, int8_t rssi) noexcept
{
    auto id = verifiedNode_(short_addr, source_ieee,
                            "presence for unknown short_addr");
    if (!id) {
        return;
    }
    if (mask != 0 || proto_version != 0) {
        (void)reg_.recordPresence(*id, short_addr, mask, proto_version);
    }
    reg_.recordRssi(*id, rssi);
}

void ZigbeeReportRouter::onChannelSample(
    uint16_t short_addr, uint64_t source_ieee,
    gh::domain::ChannelSample s, int8_t rssi) noexcept
{
    auto id = verifiedNode_(short_addr, source_ieee,
                            "sample for unknown short_addr");
    if (!id) {
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
