#pragma once
#include <string>
#include <vector>
#include "ports/IHttpClient.hpp"

namespace gh::test {

class FakeHttpClient final : public gh::domain::IHttpClient {
public:
    struct Call {
        std::string url;
        std::string api_key;
        std::string body;
        uint32_t    timeout_ms;
    };

    gh::domain::HttpResponse postJson(const char* url,
                                      const char* api_key,
                                      const char* body,
                                      size_t      body_len,
                                      uint32_t    timeout_ms) noexcept override {
        calls.push_back(Call{url, api_key, std::string(body, body_len), timeout_ms});
        return next_response;
    }

    void reply200() { next_response = {200, 0, gh::domain::ErrorCode::Ok}; }
    void reply500() { next_response = {500, 0, gh::domain::ErrorCode::Ok}; }
    void reply400() { next_response = {400, 0, gh::domain::ErrorCode::Ok}; }
    void replyNetworkDown() {
        next_response = {-1, 0, gh::domain::ErrorCode::HttpTransportFailure};
    }
    void replyRetryAfter(uint16_t s) {
        next_response = {503, s, gh::domain::ErrorCode::Ok};
    }

    std::vector<Call>        calls;
    gh::domain::HttpResponse next_response{200, 0, gh::domain::ErrorCode::Ok};
};

}
