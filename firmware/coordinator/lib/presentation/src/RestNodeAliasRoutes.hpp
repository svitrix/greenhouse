#pragma once
#ifdef ARDUINO

#include <ESPAsyncWebServer.h>
#include "ports/INodeAliasStore.hpp"
#include "ports/INodeRegistry.hpp"
#include "RestHelpers.hpp"

namespace gh::presentation {

class RestNodeAliasRoutes {
public:
    RestNodeAliasRoutes(gh::domain::INodeRegistry&     reg,
                        gh::domain::INodeAliasStore&   aliases) noexcept
        : reg_{reg}, aliases_{aliases} {}

    void registerOn(AsyncWebServer& server) noexcept;

private:
    gh::domain::INodeRegistry&     reg_;
    gh::domain::INodeAliasStore&   aliases_;
    char                           body_buf_[rest::kMaxBodyBytes] = {};
};

}  // namespace gh::presentation

#endif  // ARDUINO
