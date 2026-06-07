#pragma once
#include "ports/IClock.hpp"
#include "ports/ILogger.hpp"
#include "ports/INodeHistoryStore.hpp"
#include "ports/INodeRegistry.hpp"
#include "ports/IZigbeeReportSink.hpp"

namespace gh::infra { class ZigbeeBindingTable; }
namespace gh::app   { class AnalyticsUploader; }

namespace gh::app {

class ZigbeeReportRouter final : public gh::domain::IZigbeeReportSink {
public:
    ZigbeeReportRouter(gh::domain::INodeRegistry&     reg,
                       gh::domain::INodeHistoryStore& hist,
                       gh::infra::ZigbeeBindingTable& bind,
                       gh::domain::ILogger&           log) noexcept;

    // Pass nullptr to disable analytics fan-out.
    void setAnalyticsBridge(AnalyticsUploader* uploader,
                             gh::domain::IClock* wall_clock_ms) noexcept;

    void onDeviceAnnounced(uint16_t short_addr, uint64_t ieee)                  noexcept override;
    void onDeviceLeft     (uint16_t short_addr)                                 noexcept override;
    void onPresenceFrame  (uint16_t short_addr, uint32_t mask,
                            uint16_t proto_version, int8_t rssi_dbm)             noexcept override;
    void onChannelSample  (uint16_t short_addr, gh::domain::ChannelSample,
                            int8_t rssi_dbm)                                     noexcept override;

private:
    gh::domain::INodeRegistry&     reg_;
    gh::domain::INodeHistoryStore& hist_;
    gh::infra::ZigbeeBindingTable& bind_;
    gh::domain::ILogger&           log_;
    AnalyticsUploader*             uploader_   = nullptr;
    gh::domain::IClock*            wall_clock_ = nullptr;
};

}
