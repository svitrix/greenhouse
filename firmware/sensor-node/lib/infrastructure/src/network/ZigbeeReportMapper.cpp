#include "ZigbeeReportMapper.hpp"
#include "ZclIds.hpp"

namespace gh::infra {

ZigbeeReportMapper::ZigbeeReportMapper(gh::domain::IZigbeeEndDevice& zb,
                                       const ChannelMapping* mappings,
                                       size_t mapping_count) noexcept
    : zb_(zb), mappings_(mappings), mapping_count_(mapping_count) {}

void ZigbeeReportMapper::publish(const gh::domain::SensorReading* readings,
                                 size_t reading_count,
                                 uint32_t sensors_present_mask,
                                 uint32_t tx_timeout_ms) noexcept {
    uint8_t buf[8] = {0};
    for (size_t i = 0; i < reading_count; ++i) {
        const auto& r = readings[i];
        const ChannelMapping* m = nullptr;
        for (size_t k = 0; k < mapping_count_; ++k) {
            if (mappings_[k].channel_id_value == r.id.value) { m = &mappings_[k]; break; }
        }
        if (!m) continue;                          // unmapped id — skip silently
        if (m->expected_kind != r.kind) continue;  // sanity guard
        for (size_t a = 0; a < m->attr_count; ++a) {
            const auto& am = m->attrs[a];
            size_t sz = 0;
            am.encode(r, buf, &sz);
            (void)zb_.reportAttribute(am.endpoint, am.cluster_id, am.attribute_id,
                                       am.type, buf, sz, tx_timeout_ms);
        }
    }
    // Always emit the mask (even with empty readings) — operator must see "all sensors down".
    (void)zb_.reportAttribute(gh::protocol::kSensorEndpoint,
                              gh::protocol::kClusterBasic,
                              gh::protocol::kAttrBasicSensorsPresentMask,
                              gh::domain::ZclType::Uint32,
                              &sensors_present_mask, sizeof(sensors_present_mask),
                              tx_timeout_ms);
}

}
