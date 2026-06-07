#pragma once
#include <cstdint>
#include "entities/SensorKind.hpp"
#include "Quantity.hpp"

namespace gh::domain {

struct ChannelSample {
    SensorKind                kind;
    gh::protocol::Quantity    quantity;
    float                     value_si;       // °C, %, V — always SI
    uint32_t                  monotonic_ms;   // assigned by coordinator on receipt
};

}
