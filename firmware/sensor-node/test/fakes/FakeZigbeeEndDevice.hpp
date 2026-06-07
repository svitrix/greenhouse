#pragma once
#include <cstddef>
#include <cstdint>
#include "ports/IZigbeeEndDevice.hpp"

namespace gh::test {

class FakeZigbeeEndDevice final : public gh::domain::IZigbeeEndDevice {
public:
    gh::domain::ErrorCode start_result  = gh::domain::ErrorCode::Ok;
    gh::domain::ErrorCode report_result = gh::domain::ErrorCode::Ok;
    uint32_t              period_s      = 300;

    gh::domain::ErrorCode start(uint32_t) noexcept override {
        return start_result;
    }

    struct AttrCall {
        uint8_t  endpoint;
        uint16_t cluster_id;
        uint16_t attribute_id;
        gh::domain::ZclType type;
        uint32_t value;   // widened up to 4 bytes little-endian for assert convenience
        size_t   size;
    };
    static constexpr size_t kMaxRecordedAttrs = 32;
    AttrCall attr_calls[kMaxRecordedAttrs] = {};
    size_t   attr_call_count = 0;

    gh::domain::ErrorCode reportAttribute(
            uint8_t  ep,
            uint16_t cid,
            uint16_t aid,
            gh::domain::ZclType type,
            const void* data,
            size_t size,
            uint32_t /*tx_timeout_ms*/) noexcept override {
        if (attr_call_count >= kMaxRecordedAttrs) { return report_result; }
        AttrCall& c = attr_calls[attr_call_count++];
        c.endpoint     = ep;
        c.cluster_id   = cid;
        c.attribute_id = aid;
        c.type         = type;
        c.size         = size;
        c.value        = 0U;
        // Pack up to 4 bytes little-endian for assertion convenience.
        const uint8_t* p = static_cast<const uint8_t*>(data);
        const size_t   n = size < 4U ? size : 4U;
        for (size_t i = 0U; i < n; ++i) {
            // i is 0..3 (loop terminates at min(size, 4)) — shift amounts 0/8/16/24
            // are all safe for uint32_t and within uint8_t shift-count constraints.
            c.value |= static_cast<uint32_t>(p[i]) << (i * 8U);
        }
        return report_result;
    }

    [[nodiscard]] uint32_t reportPeriodSeconds() const noexcept override {
        return period_s;
    }
};

}
