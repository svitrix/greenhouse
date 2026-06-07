#include "MqttClient.hpp"
#include <Arduino.h>
#include <cstring>

namespace gh::infra {

MqttClient::MqttClient(const gh::domain::MqttCreds& creds) noexcept
    : creds_(creds) {
    client_.setServer(creds_.host, creds_.port);
    if (creds_.user[0] != '\0') {
        client_.setCredentials(creds_.user, creds_.password);
    }
    client_.setClientId(creds_.client_id);
    client_.setCleanSession(true);
    client_.setKeepAlive(30U);  // seconds

    // Resubscribe after every connect (clean session means the broker drops
    // our subscriptions on disconnect; the lib's session-present flag is
    // therefore always false here, but we resubscribe unconditionally).
    client_.onConnect([this](bool /*sessionPresent*/) {
        resubscribeAll();
    });

    client_.onMessage([this](const espMqttClientTypes::MessageProperties& /*props*/,
                              const char* topic,
                              const uint8_t* payload,
                              size_t len,
                              size_t /*index*/,
                              size_t /*total*/) {
        dispatchMessage(topic, payload, len);
    });
}

gh::domain::ErrorCode MqttClient::connect() noexcept {
    if (client_.connected()) return gh::domain::ErrorCode::Ok;
    if (!client_.connect()) {
        return gh::domain::ErrorCode::MqttDisconnected;
    }
    // espMqttClient::connect() returns true once the TCP request was enqueued,
    // not once the MQTT CONNACK is received. Poll briefly so that callers
    // (and HomeAssistantDiscovery::publishAll right after connect) observe a
    // consistent state. Library keeps retrying in the background if we time
    // out here.
    const uint32_t deadline = millis() + 5000U;
    while (!client_.connected() && static_cast<int32_t>(millis() - deadline) < 0) {
        delay(20);
    }
    return client_.connected() ? gh::domain::ErrorCode::Ok
                                : gh::domain::ErrorCode::MqttDisconnected;
}

bool MqttClient::isConnected() const noexcept {
    return client_.connected();
}

gh::domain::ErrorCode MqttClient::publish(std::string_view topic,
                                          std::string_view payload,
                                          bool retain) noexcept {
    if (!isConnected()) return gh::domain::ErrorCode::MqttDisconnected;
    char topic_buf[96];
    if (topic.size() >= sizeof(topic_buf)) return gh::domain::ErrorCode::Unknown;
    std::memcpy(topic_buf, topic.data(), topic.size());
    topic_buf[topic.size()] = '\0';
    const uint16_t pkt = client_.publish(
        topic_buf,
        /*qos*/ 0U,
        retain,
        reinterpret_cast<const uint8_t*>(payload.data()),
        payload.size());
    return pkt != 0U ? gh::domain::ErrorCode::Ok
                     : gh::domain::ErrorCode::MqttDisconnected;
}

gh::domain::ErrorCode MqttClient::subscribe(std::string_view topic,
                                             gh::domain::MqttMessageHandler handler,
                                             void* user_ctx) noexcept {
    for (auto& s : subs_) {
        if (!s.active) {
            if (topic.size() >= sizeof(s.topic)) return gh::domain::ErrorCode::Unknown;
            std::memcpy(s.topic, topic.data(), topic.size());
            s.topic[topic.size()] = '\0';
            s.handler  = handler;
            s.user_ctx = user_ctx;
            s.active   = true;
            if (isConnected()) (void)client_.subscribe(s.topic, /*qos*/ 0U);
            return gh::domain::ErrorCode::Ok;
        }
    }
    return gh::domain::ErrorCode::Unknown;
}

void MqttClient::resubscribeAll() noexcept {
    for (const auto& s : subs_) {
        if (s.active) (void)client_.subscribe(s.topic, /*qos*/ 0U);
    }
}

void MqttClient::dispatchMessage(const char* topic,
                                  const uint8_t* payload,
                                  size_t len) noexcept {
    const std::string_view topic_sv{topic};
    for (const auto& s : subs_) {
        if (s.active && topic_sv == s.topic && s.handler) {
            s.handler(topic_sv, payload, static_cast<uint16_t>(len), s.user_ctx);
            return;
        }
    }
}

}  // namespace gh::infra
