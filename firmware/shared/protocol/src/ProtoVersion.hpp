// firmware/shared/protocol/src/ProtoVersion.hpp
#pragma once
#include <cstdint>

namespace gh::protocol {

// Multi-node coordinator contract — bumped when the on-the-wire meaning of
// any cluster/attribute changes in a way the coordinator must distinguish.
constexpr uint16_t kProtocolVersion = 1;

// EP1 Basic, uint16, RO + reporting. Coordinator tolerates absence and
// treats missing/0 as proto_version=0.
constexpr uint16_t kAttrBasicProtoVersion = 0xF002;

}
