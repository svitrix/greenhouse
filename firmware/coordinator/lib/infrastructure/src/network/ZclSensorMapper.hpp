#pragma once
#include <cstdint>
#include <cstddef>
#include <optional>
#include "ChannelAttrTable.hpp"
#include "entities/SensorKind.hpp"

namespace gh::infra {

class ZclSensorMapper {
public:
    struct Decoded {
        gh::domain::SensorKind  kind;
        gh::protocol::Quantity  quantity;
        float                   value_si;
    };

    [[nodiscard]] static std::optional<Decoded> decode(
        uint8_t endpoint, uint16_t cluster, uint16_t attr,
        const uint8_t* raw, size_t len) noexcept;
};

}
