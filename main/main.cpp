#include <cstdio>
#include <cstring>
#include <swd_host.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <mbedtls/base64.h>
#include <swd_prog.hpp>



extern "C" void app_main(void)
{
    static const char *TAG = "main";
    ESP_LOGI(TAG, "Hello from DAPLink!");

    auto &swd = swd_prog::instance();

    ESP_ERROR_CHECK(swd.init());
    ESP_LOGI(TAG, "Inited");

    ESP_ERROR_CHECK(swd.erase_sector(0x08000000, 128, 0x08020000));
    ESP_LOGI(TAG, "Erased");

    uint8_t *buf = static_cast<uint8_t *>(malloc(8192));
    memset(buf, 0x5a, 8192);

    ESP_ERROR_CHECK(swd.program_page(0x08000080, buf, 8192));
    ESP_LOGI(TAG, "Flashed");


