#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace gh::domain {

struct NodeId {
    uint64_t ieee = 0;

    [[nodiscard]] constexpr bool operator==(NodeId o) const noexcept { return ieee == o.ieee; }
    [[nodiscard]] constexpr bool operator!=(NodeId o) const noexcept { return ieee != o.ieee; }
    [[nodiscard]] constexpr bool operator< (NodeId o) const noexcept { return ieee  < o.ieee;  }

    // 16 hex chars + NUL terminator. Uppercase. No separators.
    [[nodiscard]] std::array<char, 17> toHex16() const noexcept;

    // Case-insensitive parse. Returns nullopt on length != 16 or non-hex chars.
    [[nodiscard]] static std::optional<NodeId> parseHex16(std::string_view s) noexcept;
};

}
