#pragma once
#include <DNSServer.h>
#include <IPAddress.h>

namespace gh::infra {
class CaptiveDnsServer {
public:
    CaptiveDnsServer() noexcept = default;

    void start(const IPAddress& ap_ip) noexcept;
    void processNext() noexcept;
    void stop() noexcept;

private:
    DNSServer dns_;
    bool      running_ = false;
};
}
