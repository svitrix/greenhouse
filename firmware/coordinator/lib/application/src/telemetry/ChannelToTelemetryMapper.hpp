#pragma once
#include <cstdint>
#include <optional>
#include "entities/ChannelSample.hpp"
#include "entities/NodeId.hpp"
#include "entities/TelemetryRecord.hpp"

namespace gh::app {

class ChannelToTelemetryMapper {
public:
    [[nodiscard]] static std::optional<gh::domain::TelemetryRecord>
        map(gh::domain::NodeId,
            const gh::domain::ChannelSample&,
            uint64_t unix_ts_ms) noexcept;
};

}
