#pragma once
#include <functional>
#include <string>
#include "ports/IMqttClient.hpp"

namespace gh::presentation {

// Subscribes to greenhouse/<device_id>/pump/cmd. On "ON" payload calls
// on_handler; on "OFF" calls off_handler. Unknown payloads are silently
// dropped. Handlers are std::function<void()> so the composition root
// can wire them through IrrigationService (which enforces the 20 s
// safety cap) without this class taking a dependency on the service.
class MqttCommandRouter {
public:
    using Handler = std::function<void()>;

    MqttCommandRouter(gh::domain::IMqttClient& mqtt,
                      std::string              device_id,
                      Handler                  on_handler,
                      Handler                  off_handler) noexcept;

    // Call once on MQTT connect.
    void subscribe() noexcept;

private:
    static void onMessage(std::string_view topic,
                          const uint8_t*   payload,
                          uint16_t         len,
                          void*            user_ctx);

    gh::domain::IMqttClient& mqtt_;
    std::string              device_id_;
    std::string              cmd_topic_;
    Handler                  on_handler_;
    Handler                  off_handler_;
};

}
