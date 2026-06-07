#include "CaptiveDnsServer.hpp"

namespace gh::infra {

void CaptiveDnsServer::start(const IPAddress& ap_ip) noexcept {
    dns_.setErrorReplyCode(DNSReplyCode::NoError);
    dns_.setTTL(0);
    dns_.start(/*port*/ 53, /*all-domains*/ "*", ap_ip);
    running_ = true;
}

void CaptiveDnsServer::processNext() noexcept {
    if (running_) dns_.processNextRequest();
}

void CaptiveDnsServer::stop() noexcept {
    dns_.stop();
    running_ = false;
}

}
