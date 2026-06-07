#pragma once
#ifdef ARDUINO

#include <ESPAsyncWebServer.h>
#include "ports/IClock.hpp"
#include "ports/INodeAliasStore.hpp"
#include "ports/INodeRegistry.hpp"

namespace gh::presentation {

class RestNodesRoutes {
public:
    RestNodesRoutes(gh::domain::INodeRegistry&     reg,
                    gh::domain::INodeAliasStore&   aliases,
                    gh::domain::IClock&            clock,
                    AsyncAuthenticationMiddleware& auth) noexcept
        : reg_{reg}, aliases_{aliases}, clock_{clock}, auth_{auth} {}

    void registerOn(AsyncWebServer& server) noexcept;

private:
    gh::domain::INodeRegistry&     reg_;
    gh::domain::INodeAliasStore&   aliases_;
    gh::domain::IClock&            clock_;
    AsyncAuthenticationMiddleware& auth_;
};

}  // namespace gh::presentation

#endif  // ARDUINO
