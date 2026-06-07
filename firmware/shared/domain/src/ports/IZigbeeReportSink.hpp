#pragma once
#include <cstdint>
#include "entities/ChannelSample.hpp"

namespace gh::domain {

class IZigbeeReportSink {
public:
    virtual ~IZigbeeReportSink() = default;

    // ZDO Device_Announce: source short_addr is known, IEEE arrives in-band.
    virtual void onDeviceAnnounced(uint16_t short_addr, uint64_t ieee) noexcept = 0;

    // ZDO Device_Leave or Mgmt_Leave_rsp confirmed: short_addr is released.
    virtual void onDeviceLeft     (uint16_t short_addr)                noexcept = 0;

    // EP1 Basic 0xF001 (uint32 sensors_present_mask) or 0xF002 (uint16 proto_version).
    // If the frame carried mask, pass mask + proto_version=0. If proto_version,
    // pass mask=0 + the version. Adapter splits the two attribute IDs at decode.
    virtual void onPresenceFrame(uint16_t short_addr, uint32_t mask,
                                  uint16_t proto_version, int8_t rssi_dbm) noexcept = 0;

    // Any other channel attribute we know (decoded to SI by ZclSensorMapper).
    virtual void onChannelSample(uint16_t short_addr,
                                  gh::domain::ChannelSample,
                                  int8_t rssi_dbm) noexcept = 0;
};

}
