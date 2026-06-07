#pragma once
#include <cstdint>
#include <string_view>
#include "errors/ErrorCode.hpp"

namespace gh::domain {

// Callback fired when a subscribed topic delivers a new payload.
// payload is NOT null-terminated; length is explicit.
using MqttMessageHandler = void(*)(std::string_view topic,
                                   const uint8_t* payload,
                                   uint16_t payload_len,
                                   void* user_ctx);

struct IMqttClient {
    virtual ~IMqttClient() = default;

    // Connect to broker; idempotent. Returns Ok if already connected.
    [[nodiscard]] virtual ErrorCode connect() noexcept = 0;

    [[nodiscard]] virtual bool isConnected() const noexcept = 0;

    // Publish a UTF-8 payload to topic. retain=true persists last value on broker.
    [[nodiscard]] virtual ErrorCode publish(std::string_view topic,
                                            std::string_view payload,
                                            bool retain) noexcept = 0;

    // Subscribe and register the handler. Handler fires from loop() context.
    [[nodiscard]] virtual ErrorCode subscribe(std::string_view topic,
                                              MqttMessageHandler handler,
                                              void* user_ctx) noexcept = 0;

    // Drive client state machine (call from a FreeRTOS task at ~10 Hz).
    virtual void loop() noexcept = 0;
};

}
