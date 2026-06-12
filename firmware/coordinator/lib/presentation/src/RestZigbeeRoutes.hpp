#pragma once
#ifdef ARDUINO

#include <ESPAsyncWebServer.h>
#include "ports/IZigbeeNetwork.hpp"
#include "RestHelpers.hpp"

namespace gh::presentation {

class RestZigbeeRoutes {
public:
    explicit RestZigbeeRoutes(gh::domain::IZigbeeNetwork& zb) noexcept
        : zb_{zb} {}

    void registerOn(AsyncWebServer&) noexcept;

private:
    gh::domain::IZigbeeNetwork&    zb_;
    char                           body_buf_[rest::kMaxBodyBytes] = {};
};

}  // namespace gh::presentation

#endif  // ARDUINO
