#pragma once
#include <cstdint>
#include <etl/vector.h>
#include "entities/ChannelSample.hpp"
#include "entities/NodeId.hpp"

namespace gh::domain {

inline constexpr size_t kMaxChannelSamplesPerNode = 8;

struct NodeSnapshot {
    NodeId    id;
    uint16_t  short_addr           = 0xFFFF;
    uint32_t  present_mask         = 0;
    uint16_t  proto_version        = 0;
    bool      proto_version_mismatch = false;
    bool      online               = false;
    uint32_t  last_seen_ms         = 0;
    int8_t    last_rssi_dbm        = INT8_MIN;
    etl::vector<ChannelSample, kMaxChannelSamplesPerNode> samples;
};

}
