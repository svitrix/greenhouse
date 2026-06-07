#pragma once
#ifdef ARDUINO

#include <ESPAsyncWebServer.h>
#include "ports/IZigbeeNetwork.hpp"

namespace gh::presentation {

class RestZigbeeRoutes {
public:
    RestZigbeeRoutes(gh::domain::IZigbeeNetwork&    zb,
                       AsyncAuthenticationMiddleware& auth) noexcept
        : zb_{zb}, auth_{auth} {}

    void registerOn(AsyncWebServer&) noexcept;

private:
    gh::domain::IZigbeeNetwork&    zb_;
    AsyncAuthenticationMiddleware& auth_;
};

}  // namespace gh::presentation

#endif  // ARDUINO
