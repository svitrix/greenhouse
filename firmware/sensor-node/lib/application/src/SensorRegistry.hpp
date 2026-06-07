#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include "ports/ISensorChannel.hpp"
#include "ports/IPowerRail.hpp"
#include "ports/ILogger.hpp"

namespace gh::app {

constexpr size_t kMaxSensorChannels = 8;

// Lightweight span replacement so the registry doesn't depend on etl/abseil.
struct ChannelSpan {
    gh::domain::ISensorChannel* const* data;
    size_t                             size;
};

class SensorRegistry {
public:
    // Returns false if registry is full (sensor not added). add() never
    // throws — embedded build is -fno-exceptions.
    bool add(gh::domain::ISensorChannel& ch) noexcept;

    [[nodiscard]] ChannelSpan channels() const noexcept;

    // Power rail on -> probe each in order -> rail off. Returns count of Ok.
    size_t probeAll(gh::domain::IPowerRail& rail, gh::domain::ILogger& log) noexcept;

    // max(warmupMs()) across channels whose status() == Ok. 0 if none Ok.
    [[nodiscard]] uint32_t maxWarmupMs() const noexcept;

    // Bit (channel.id().value) set iff channel.status() == Ok.
    [[nodiscard]] uint32_t presentMask() const noexcept;

private:
    std::array<gh::domain::ISensorChannel*, kMaxSensorChannels> channels_{};
    size_t count_ = 0;
};

}
