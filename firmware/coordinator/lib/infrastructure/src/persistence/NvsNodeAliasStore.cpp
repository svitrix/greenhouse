#include "NvsNodeAliasStore.hpp"

namespace gh::infra {

gh::domain::ErrorCode NvsNodeAliasStore::begin() noexcept {
    if (!prefs_.begin("nodes_alias", false)) {
        return gh::domain::ErrorCode::ConfigStoreFailed;
    }
    opened_ = true;
    return gh::domain::ErrorCode::Ok;
}

gh::domain::ErrorCode NvsNodeAliasStore::setAlias(
    gh::domain::NodeId id, std::string_view a) noexcept
{
    if (!opened_)                                  return gh::domain::ErrorCode::SensorNotReady;
    if (a.size() > gh::domain::kMaxAliasBytes)     return gh::domain::ErrorCode::AliasTooLong;
    // Reject control chars / malformed UTF-8 before it can reach the SPA.
    if (!gh::domain::isValidAlias(a))              return gh::domain::ErrorCode::InvalidArgument;
    const auto key = id.toHex16();
    if (prefs_.putBytes(key.data(), a.data(), a.size()) != a.size()) {
        return gh::domain::ErrorCode::ConfigStoreFailed;
    }
    return gh::domain::ErrorCode::Ok;
}

gh::domain::ErrorCode NvsNodeAliasStore::clearAlias(gh::domain::NodeId id) noexcept {
    if (!opened_) return gh::domain::ErrorCode::SensorNotReady;
    const auto key = id.toHex16();
    prefs_.remove(key.data());
    return gh::domain::ErrorCode::Ok;
}

std::optional<std::array<char, gh::domain::kMaxAliasBytes + 1>>
NvsNodeAliasStore::alias(gh::domain::NodeId id) const noexcept
{
    if (!opened_) return std::nullopt;
    const auto key = id.toHex16();
    const size_t len = prefs_.getBytesLength(key.data());
    if (len == 0 || len > gh::domain::kMaxAliasBytes) return std::nullopt;
    std::array<char, gh::domain::kMaxAliasBytes + 1> out{};
    const size_t got = prefs_.getBytes(key.data(), out.data(), len);
    if (got != len) return std::nullopt;
    out[len] = '\0';
    return out;
}

}
