#pragma once
#include <cassert>
#include <limits>
#include <string>
#include <vector>
#include "ports/IMqttClient.hpp"

namespace gh::test {

struct PublishedMessage {
    std::string topic;
    std::string payload;
    bool retain;
};

struct Subscription {
    std::string topic;
    gh::domain::MqttMessageHandler handler;
    void* user_ctx;
};

class FakeMqttClient final : public gh::domain::IMqttClient {
public:
    bool connected = true;  // tests can flip this to simulate disconnect
    std::vector<PublishedMessage> published;
    std::vector<Subscription>     subscriptions;
    int loop_calls = 0;

    gh::domain::ErrorCode connect() noexcept override {
        connected = true;
        return gh::domain::ErrorCode::Ok;
    }

    bool isConnected() const noexcept override { return connected; }

    gh::domain::ErrorCode publish(std::string_view topic,
                                  std::string_view payload,
                                  bool retain) noexcept override {
        if (!connected) return gh::domain::ErrorCode::MqttDisconnected;
        published.push_back({std::string(topic), std::string(payload), retain});
        return gh::domain::ErrorCode::Ok;
    }

    gh::domain::ErrorCode subscribe(std::string_view topic,
                                    gh::domain::MqttMessageHandler handler,
                                    void* user_ctx) noexcept override {
        subscriptions.push_back({std::string(topic), handler, user_ctx});
        return gh::domain::ErrorCode::Ok;
    }

    void loop() noexcept override { loop_calls++; }

    // Resets all observed state — call between test cases that reuse the same fake.
    void reset() noexcept {
        published.clear();
        subscriptions.clear();
        loop_calls = 0;
        connected  = true;
    }

    // Test helper: simulate broker delivering a message to our subscription.
    // Topic matching is exact — no MQTT wildcard support (#, +).
    void deliverMessage(std::string_view topic, std::string_view payload) {
        assert(payload.size() <= std::numeric_limits<uint16_t>::max());
        for (auto& sub : subscriptions) {
            if (sub.topic == topic) {
                sub.handler(topic,
                            reinterpret_cast<const uint8_t*>(payload.data()),
                            static_cast<uint16_t>(payload.size()),
                            sub.user_ctx);
            }
        }
    }
};

}
