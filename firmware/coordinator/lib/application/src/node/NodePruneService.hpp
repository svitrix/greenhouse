#pragma once
#include <cstdint>
#include "ports/IClock.hpp"
#include "ports/INodeRegistry.hpp"

namespace gh::app {

class NodePruneService {
public:
    NodePruneService(gh::domain::INodeRegistry& reg,
                     gh::domain::IClock& clock,
                     uint32_t offline_threshold_ms) noexcept
        : reg_{reg}, clock_{clock}, offline_threshold_ms_{offline_threshold_ms} {}

    void tick() noexcept;

private:
    gh::domain::INodeRegistry& reg_;
    gh::domain::IClock&        clock_;
    uint32_t                   offline_threshold_ms_;
};

}
