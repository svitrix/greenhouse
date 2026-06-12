#pragma once
#ifdef ARDUINO

#include <ESPAsyncWebServer.h>
#include "ports/IClock.hpp"
#include "ports/INodeHistoryStore.hpp"

namespace gh::presentation {

class RestHistoryRoutes {
public:
    RestHistoryRoutes(gh::domain::INodeHistoryStore& hist,
                        gh::domain::IClock&            clock) noexcept
        : hist_{hist}, clock_{clock} {}

    void registerOn(AsyncWebServer& server) noexcept;

private:
    gh::domain::INodeHistoryStore& hist_;
    gh::domain::IClock&            clock_;
};

}  // namespace gh::presentation

#endif  // ARDUINO
