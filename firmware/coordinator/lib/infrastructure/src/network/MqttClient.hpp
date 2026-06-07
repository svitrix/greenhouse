#pragma once
#include <espMqttClientAsync.h>
#include <array>
#include "ports/IMqttClient.hpp"
#include "entities/MqttCreds.hpp"

namespace gh::infra {

constexpr size_t kMaxSubscriptions = 4;

class MqttClient final : public gh::domain::IMqttClient {
public:
    explicit MqttClient(const gh::domain::MqttCreds& creds) noexcept;

    gh::domain::ErrorCode connect()         noexcept override;
    bool                  isConnected() const noexcept override;
    gh::domain::ErrorCode publish(std::string_view topic,
                                   std::string_view payload,
                                   bool retain) noexcept override;
    gh::domain::ErrorCode subscribe(std::string_view topic,
                                     gh::domain::MqttMessageHandler handler,
                                     void* user_ctx) noexcept override;
    // No-op: espMqttClient drives itself on the AsyncTCP task.
    void loop() noexcept override {}

private:
    struct Sub {
        char topic[64];
        gh::domain::MqttMessageHandler handler;
        void* user_ctx;
        bool active;
    };

    gh::domain::MqttCreds              creds_;
    espMqttClientAsync                 client_;
    std::array<Sub, kMaxSubscriptions> subs_{};

    void resubscribeAll() noexcept;
    void dispatchMessage(const char* topic,
                         const uint8_t* payload,
                         size_t len) noexcept;
};

}  // namespace gh::infra
