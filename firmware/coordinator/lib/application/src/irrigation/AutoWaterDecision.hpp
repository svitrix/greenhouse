#pragma once
#include <cstdint>
#include <optional>
#include <etl/vector.h>
#include "entities/NodeId.hpp"
#include "ports/INodeRegistry.hpp"  // kMaxRegisteredNodes

namespace gh::app {

enum class AutoWaterOutcome : uint8_t {
    Started,
    Stopped,
    SkipAboveThreshold,
    LockNoFreshSoil,
    LockInsufficientSources,
    LockMaxRuntime,
    LockMinInterval,
    LockFloatSwitch,
    Disabled,
};

struct AutoWaterDecision {
    AutoWaterOutcome                                      outcome = AutoWaterOutcome::Disabled;
    std::optional<float>                                  avg_moisture_pct;
    etl::vector<gh::domain::NodeId,
                gh::domain::kMaxRegisteredNodes>          fresh_sources;
    etl::vector<gh::domain::NodeId,
                gh::domain::kMaxRegisteredNodes>          stale_sources;
    uint32_t                                              monotonic_ms = 0;
};

[[nodiscard]] const char* outcomeCode(AutoWaterOutcome) noexcept;

}
