#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include "entities/NodeId.hpp"
#include "errors/ErrorCode.hpp"

namespace gh::domain {

inline constexpr size_t kMaxAliasBytes = 23;     // + 1 NUL = 24 bytes

class INodeAliasStore {
public:
    virtual ~INodeAliasStore() = default;

    // ErrorCode::AliasTooLong if alias.size() > kMaxAliasBytes.
    [[nodiscard]] virtual ErrorCode setAlias  (NodeId, std::string_view alias) noexcept = 0;
    [[nodiscard]] virtual ErrorCode clearAlias(NodeId)                          noexcept = 0;
    [[nodiscard]] virtual std::optional<std::array<char, kMaxAliasBytes + 1>>
                          alias(NodeId) const noexcept = 0;
};

}
