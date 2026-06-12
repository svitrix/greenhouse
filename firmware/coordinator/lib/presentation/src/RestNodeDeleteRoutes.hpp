#pragma once
#ifdef ARDUINO

#include <ESPAsyncWebServer.h>
#include "ports/INodeAliasStore.hpp"
#include "ports/INodeHistoryStore.hpp"
#include "ports/INodeRegistry.hpp"
#include "ports/IZigbeeNetwork.hpp"

namespace gh::presentation {

class RestNodeDeleteRoutes {
public:
    RestNodeDeleteRoutes(gh::domain::INodeRegistry&     reg,
                         gh::domain::INodeAliasStore&   aliases,
                         gh::domain::INodeHistoryStore& hist,
                         gh::domain::IZigbeeNetwork&    zb) noexcept
        : reg_{reg}, aliases_{aliases}, hist_{hist}, zb_{zb} {}

    void registerOn(AsyncWebServer& server) noexcept;

private:
    gh::domain::INodeRegistry&     reg_;
    gh::domain::INodeAliasStore&   aliases_;
    gh::domain::INodeHistoryStore& hist_;
    gh::domain::IZigbeeNetwork&    zb_;
};

}  // namespace gh::presentation

#endif  // ARDUINO
