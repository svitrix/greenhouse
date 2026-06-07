#include "entities/NodeId.hpp"

namespace gh::domain {

std::array<char, 17> NodeId::toHex16() const noexcept {
    static constexpr char kHex[] = "0123456789ABCDEF";
    std::array<char, 17> out{};
    uint64_t v = ieee;
    for (int i = 15; i >= 0; --i) {
        out[static_cast<size_t>(i)] = kHex[v & 0xFu];
        v >>= 4;
    }
    out[16] = '\0';
    return out;
}

std::optional<NodeId> NodeId::parseHex16(std::string_view s) noexcept {
    if (s.size() != 16) return std::nullopt;
    uint64_t v = 0;
    for (char c : s) {
        uint8_t n;
        if      (c >= '0' && c <= '9') n = static_cast<uint8_t>(c - '0');
        else if (c >= 'a' && c <= 'f') n = static_cast<uint8_t>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') n = static_cast<uint8_t>(c - 'A' + 10);
        else return std::nullopt;
        v = (v << 4) | n;
    }
    return NodeId{v};
}

}
