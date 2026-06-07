#pragma once
#include <ArduinoJson.h>
#include "entities/NodeSnapshot.hpp"
#include "ports/INodeAliasStore.hpp"

namespace gh::presentation {

class NodeViewBuilder {
public:
    static void build(const gh::domain::NodeSnapshot& snap,
                       const gh::domain::INodeAliasStore& aliases,
                       uint32_t now_ms,
                       JsonObject out) noexcept;
};

}
