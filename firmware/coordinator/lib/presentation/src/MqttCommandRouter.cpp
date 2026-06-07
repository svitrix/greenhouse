#include "MqttCommandRouter.hpp"
#include <cstring>

namespace gh::presentation {

MqttCommandRouter::MqttCommandRouter(gh::domain::IMqttClient& mqtt,
                                     std::string device_id,
                                     Handler on_handler,
                                     Handler off_handler) noexcept
    : mqtt_(mqtt),
      device_id_(std::move(device_id)),
      cmd_topic_("greenhouse/" + device_id_ + "/pump/cmd"),
      on_handler_(std::move(on_handler)),
      off_handler_(std::move(off_handler)) {
}

void MqttCommandRouter::subscribe() noexcept {
    (void)mqtt_.subscribe(cmd_topic_, &MqttCommandRouter::onMessage, this);
}

void MqttCommandRouter::onMessage(std::string_view /*topic*/,
                                  const uint8_t* payload,
                                  uint16_t       len,
                                  void*          user_ctx) {
    auto* self = static_cast<MqttCommandRouter*>(user_ctx);
    if (len == 2 && std::memcmp(payload, "ON", 2) == 0) {
        if (self->on_handler_) self->on_handler_();
    } else if (len == 3 && std::memcmp(payload, "OFF", 3) == 0) {
        if (self->off_handler_) self->off_handler_();
    }
    // Else: silently ignore unknown payloads.
}

}
