#pragma once
#include <cstddef>
#include "ChannelMappings.hpp"
#include "entities/SensorReading.hpp"
#include "ports/IZigbeeEndDevice.hpp"

namespace gh::infra {

class ZigbeeReportMapper {
public:
    ZigbeeReportMapper(gh::domain::IZigbeeEndDevice& zb,
                       const ChannelMapping* mappings,
                       size_t mapping_count) noexcept;

    // Convenience overload for the constexpr array literal.
    template<size_t N>
    ZigbeeReportMapper(gh::domain::IZigbeeEndDevice& zb,
                       const ChannelMapping (&mappings)[N]) noexcept
        : ZigbeeReportMapper(zb, mappings, N) {}

    // For each reading: look up mapping by id; for each attribute in the
    // mapping, call zb_.reportAttribute(...). After all readings: publish
    // the sensors_present_mask attribute on EP1 Basic@0xF001.
    void publish(const gh::domain::SensorReading* readings,
                 size_t reading_count,
                 uint32_t sensors_present_mask,
                 uint32_t tx_timeout_ms) noexcept;

private:
    gh::domain::IZigbeeEndDevice& zb_;
    const ChannelMapping*         mappings_;
    size_t                        mapping_count_;
};

}
