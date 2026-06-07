#pragma once
#include <optional>
#include <string_view>
#include "Quantity.hpp"
#include "entities/NodeId.hpp"
#include "entities/SensorKind.hpp"

namespace gh::presentation {

// Extracts the first 16-hex-char segment from a path like
// "/api/nodes/<IEEE>" or "/api/nodes/<IEEE>/alias".
[[nodiscard]] std::optional<gh::domain::NodeId>
    parseIeeeFromPath(std::string_view path) noexcept;

[[nodiscard]] const char* kindCode(gh::domain::SensorKind) noexcept;
[[nodiscard]] std::optional<gh::domain::SensorKind>
    kindFromCode(std::string_view) noexcept;
[[nodiscard]] std::optional<gh::protocol::Quantity>
    quantityFromCode(std::string_view) noexcept;

}
