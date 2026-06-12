#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include "entities/NodeId.hpp"
#include "errors/ErrorCode.hpp"

namespace gh::domain {

inline constexpr size_t kMaxAliasBytes = 23;     // + 1 NUL = 24 bytes

// A node alias is rendered verbatim in the admin SPA, so it must be safe to
// embed in HTML/JSON: no C0/C1 control characters (NUL, CR, LF, DEL, ...) and
// well-formed UTF-8 (no truncated / overlong / stray continuation bytes, no
// surrogates, nothing above U+10FFFF). Empty is rejected — callers use
// clearAlias() to remove an alias instead of storing "".
[[nodiscard]] inline bool isValidAlias(std::string_view a) noexcept {
    if (a.empty() || a.size() > kMaxAliasBytes) return false;

    size_t i = 0;
    while (i < a.size()) {
        const auto b0 = static_cast<uint8_t>(a[i]);
        if (b0 < 0x20 || b0 == 0x7F) return false;          // C0 controls + DEL

        size_t  extra = 0;
        uint8_t lower = 0x80;
        uint8_t upper = 0xBF;
        if (b0 < 0x80) {
            extra = 0;                                       // ASCII
        } else if ((b0 & 0xE0) == 0xC0) {
            if (b0 < 0xC2) return false;                     // overlong 2-byte / 0xC0-0xC1
            extra = 1;
        } else if ((b0 & 0xF0) == 0xE0) {
            extra = 2;
            if (b0 == 0xE0) lower = 0xA0;                    // no overlong
            if (b0 == 0xED) upper = 0x9F;                    // no UTF-16 surrogates
        } else if ((b0 & 0xF8) == 0xF0) {
            if (b0 > 0xF4) return false;                     // > U+10FFFF
            extra = 3;
            if (b0 == 0xF0) lower = 0x90;                    // no overlong
            if (b0 == 0xF4) upper = 0x8F;                    // no > U+10FFFF
        } else {
            return false;                                    // stray continuation / 0xF5+
        }

        if (i + extra >= a.size()) return false;             // truncated sequence
        for (size_t k = 1; k <= extra; ++k) {
            const auto bn = static_cast<uint8_t>(a[i + k]);
            const uint8_t lo = (k == 1) ? lower : 0x80;
            const uint8_t hi = (k == 1) ? upper : 0xBF;
            if (bn < lo || bn > hi) return false;
        }

        // C1 controls (U+0080..U+009F) encode as 0xC2 0x80..0x9F — reject.
        if (b0 == 0xC2 && static_cast<uint8_t>(a[i + 1]) <= 0x9F) return false;

        i += extra + 1;
    }
    return true;
}

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
