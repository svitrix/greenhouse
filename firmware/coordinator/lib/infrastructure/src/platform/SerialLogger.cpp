#include "SerialLogger.hpp"
#include <esp_log.h>

namespace gh::infra {

void SerialLogger::info(const char* tag, const char* msg) noexcept {
    ESP_LOGI(tag, "%s", msg);
}
void SerialLogger::warn(const char* tag, const char* msg) noexcept {
    ESP_LOGW(tag, "%s", msg);
}
void SerialLogger::error(const char* tag, const char* msg) noexcept {
    ESP_LOGE(tag, "%s", msg);
}

}
