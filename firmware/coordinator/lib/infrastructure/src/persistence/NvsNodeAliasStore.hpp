#pragma once
#include <Preferences.h>
#include "ports/INodeAliasStore.hpp"

namespace gh::infra {

class NvsNodeAliasStore final : public gh::domain::INodeAliasStore {
public:
    [[nodiscard]] gh::domain::ErrorCode begin() noexcept;     // call once at boot
    [[nodiscard]] gh::domain::ErrorCode setAlias  (gh::domain::NodeId,
                                                    std::string_view) noexcept override;
    [[nodiscard]] gh::domain::ErrorCode clearAlias(gh::domain::NodeId) noexcept override;
    [[nodiscard]] std::optional<std::array<char, gh::domain::kMaxAliasBytes + 1>>
                  alias(gh::domain::NodeId) const noexcept override;
private:
    mutable Preferences prefs_;
    bool opened_ = false;
};

}
