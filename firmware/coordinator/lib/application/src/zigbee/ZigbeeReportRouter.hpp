#pragma once
#include <optional>
#include "entities/NodeId.hpp"
#include "ports/IClock.hpp"
#include "ports/ILogger.hpp"
#include "ports/INodeHistoryStore.hpp"
#include "ports/INodeRegistry.hpp"
#include "ports/IShortAddrResolver.hpp"
#include "ports/IZigbeeReportSink.hpp"

namespace gh::app   { class AnalyticsUploader; }

namespace gh::app {

class ZigbeeReportRouter final : public gh::domain::IZigbeeReportSink {
public:
    ZigbeeReportRouter(gh::domain::INodeRegistry&        reg,
                       gh::domain::INodeHistoryStore&    hist,
                       gh::domain::IShortAddrResolver&   bind,
                       gh::domain::ILogger&              log) noexcept;

    // Pass nullptr to disable analytics fan-out.
    void setAnalyticsBridge(AnalyticsUploader* uploader,
                             gh::domain::IClock* wall_clock_ms) noexcept;

    void onDeviceAnnounced(uint16_t short_addr, uint64_t ieee)                  noexcept override;
    void onDeviceLeft     (uint16_t short_addr)                                 noexcept override;
    void onPresenceFrame  (uint16_t short_addr, uint64_t source_ieee,
                            uint32_t mask, uint16_t proto_version,
                            int8_t rssi_dbm)                                     noexcept override;
    void onChannelSample  (uint16_t short_addr, uint64_t source_ieee,
                            gh::domain::ChannelSample, int8_t rssi_dbm)          noexcept override;

private:
    // Resolves short_addr → bound NodeId and rejects a spoofed frame whose APS
    // source IEEE (source_ieee) does not match the binding. Returns the bound
    // NodeId on success, nullopt when unknown or mismatched (already logged).
    [[nodiscard]] std::optional<gh::domain::NodeId>
        verifiedNode_(uint16_t short_addr, uint64_t source_ieee,
                      const char* ctx) noexcept;

    gh::domain::INodeRegistry&      reg_;
    gh::domain::INodeHistoryStore&  hist_;
    gh::domain::IShortAddrResolver& bind_;
    gh::domain::ILogger&            log_;
    AnalyticsUploader*              uploader_   = nullptr;
    gh::domain::IClock*             wall_clock_ = nullptr;
};

}
