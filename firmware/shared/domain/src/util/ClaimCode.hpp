#pragma once
#include <cstddef>
#include <string_view>

namespace gh::domain {

// Hue-style pairing claim code: EXACTLY 6 ASCII decimal digits ('0'..'9').
// Rejecting anything else server-side is what makes it safe to embed the code
// in a JSON body — a value like 12"}} can no longer break out of the string.
constexpr std::size_t kClaimCodeLen = 6;

[[nodiscard]] constexpr bool isValidClaimCode(std::string_view code) noexcept {
    if (code.size() != kClaimCodeLen) {
        return false;
    }
    for (const char c : code) {
        if (c < '0' || c > '9') {
            return false;
        }
    }
    return true;
}

}
