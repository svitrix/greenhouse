#include "ZclSensorMapper.hpp"
#include "ZclIds.hpp"

namespace gh::infra {

static uint16_t readU16LE(const uint8_t* p) noexcept {
    return static_cast<uint16_t>(p[0]) |
           static_cast<uint16_t>(static_cast<uint16_t>(p[1]) << 8u);
}
static int16_t readI16LE(const uint8_t* p) noexcept {
    return static_cast<int16_t>(readU16LE(p));
}

std::optional<ZclSensorMapper::Decoded>
ZclSensorMapper::decode(uint8_t ep, uint16_t cluster, uint16_t attr,
                        const uint8_t* raw, size_t len) noexcept
{
    const auto* e = gh::protocol::findByZclAddress(ep, cluster, attr);
    if (e == nullptr) return std::nullopt;

    switch (e->scalar) {
        case gh::protocol::ZclScalar::Int16: {
            if (len < 2) return std::nullopt;
            const int16_t v = readI16LE(raw);
            switch (e->quantity) {
                case gh::protocol::Quantity::AirTempC:
                    return Decoded{e->kind, e->quantity,
                        static_cast<float>(gh::protocol::airTempFromZcl(v)) / 10.0f};
                case gh::protocol::Quantity::SoilTempC:
                    return Decoded{e->kind, e->quantity,
                        static_cast<float>(gh::protocol::airTempFromZcl(v)) / 10.0f};
                default: return std::nullopt;
            }
        }
        case gh::protocol::ZclScalar::Uint16: {
            if (len < 2) return std::nullopt;
            const uint16_t v = readU16LE(raw);
            switch (e->quantity) {
                case gh::protocol::Quantity::AirHumidityPct:
                    return Decoded{e->kind, e->quantity,
                        static_cast<float>(gh::protocol::airHumidityFromZcl(v)) / 10.0f};
                case gh::protocol::Quantity::SoilMoisturePct:
                    return Decoded{e->kind, e->quantity,
                        static_cast<float>(v) / 100.0f};
                default: return std::nullopt;
            }
        }
        case gh::protocol::ZclScalar::Uint8: {
            if (len < 1) return std::nullopt;
            const uint8_t v = raw[0];
            switch (e->quantity) {
                case gh::protocol::Quantity::BatteryPct:
                    return Decoded{e->kind, e->quantity,
                        static_cast<float>(gh::protocol::batteryPctFromZcl(v))};
                case gh::protocol::Quantity::BatteryVoltageV:
                    return Decoded{e->kind, e->quantity,
                        static_cast<float>(v) / 10.0f};
                default: return std::nullopt;
            }
        }
    }
    return std::nullopt;
}

}
