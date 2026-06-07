#pragma once
#include <cstring>
#include <etl/flat_map.h>
#include "ports/INodeAliasStore.hpp"
#include "ports/INodeRegistry.hpp"

namespace gh::test {

class InMemoryAliasStore final : public gh::domain::INodeAliasStore {
public:
    [[nodiscard]] gh::domain::ErrorCode setAlias(
        gh::domain::NodeId id, std::string_view a) noexcept override {
        if (a.size() > gh::domain::kMaxAliasBytes) {
            return gh::domain::ErrorCode::AliasTooLong;
        }
        Buf b{};
        for (size_t i = 0; i < a.size(); ++i) b[i] = a[i];
        b[a.size()] = '\0';
        map_[id] = b;
        return gh::domain::ErrorCode::Ok;
    }
    [[nodiscard]] gh::domain::ErrorCode clearAlias(
        gh::domain::NodeId id) noexcept override {
        map_.erase(id);
        return gh::domain::ErrorCode::Ok;
    }
    [[nodiscard]] std::optional<std::array<char, gh::domain::kMaxAliasBytes + 1>>
        alias(gh::domain::NodeId id) const noexcept override {
        auto it = map_.find(id);
        if (it == map_.end()) return std::nullopt;
        return it->second;
    }
private:
    using Buf = std::array<char, gh::domain::kMaxAliasBytes + 1>;
    etl::flat_map<gh::domain::NodeId, Buf,
                  gh::domain::kMaxRegisteredNodes> map_;
};

}
