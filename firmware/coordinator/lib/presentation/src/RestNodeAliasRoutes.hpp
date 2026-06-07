#pragma once
#ifdef ARDUINO

#include <ESPAsyncWebServer.h>
#include "ports/INodeAliasStore.hpp"
#include "ports/INodeRegistry.hpp"

namespace gh::presentation {

class RestNodeAliasRoutes {
public:
    RestNodeAliasRoutes(gh::domain::INodeRegistry&     reg,
                        gh::domain::INodeAliasStore&   aliases,
                        AsyncAuthenticationMiddleware& auth) noexcept
        : reg_{reg}, aliases_{aliases}, auth_{auth} {}

    void registerOn(AsyncWebServer& server) noexcept;

private:
    gh::domain::INodeRegistry&     reg_;
    gh::domain::INodeAliasStore&   aliases_;
    AsyncAuthenticationMiddleware& auth_;
};

}  // namespace gh::presentation

#endif  // ARDUINO
