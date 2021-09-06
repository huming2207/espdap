#include <cstdio>
#include <cstring>
#include <swd_host.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <mbedtls/base64.h>
#include <swd_prog.hpp>



extern "C" void app_main(void) {
    static const char *TAG = "main";
    ESP_LOGI(TAG, "Hello from DAPLink!");

    auto &swd = swd_prog::instance();

    ESP_ERROR_CHECK(swd.init());
    ESP_LOGI(TAG, "Inited");

    ESP_ERROR_CHECK(swd.erase_sector(0x08000000, 128, 0x08020000));
    ESP_LOGI(TAG, "Erased");

    uint8_t *buf = static_cast<uint8_t *>(malloc(10240));
    memset(buf, 0xa5, 10240);
    swd_read_memory(0x08000000, buf, 1024);
    ESP_LOG_BUFFER_HEX(TAG, buf, 1024);


    memset(buf, 0x5a, 1024);

    ESP_ERROR_CHECK(swd.init());
    ESP_LOGI(TAG, "Re-Inited");

    int64_t ts = esp_timer_get_time();
    ESP_ERROR_CHECK(swd.program_page(0x08000000, buf, 10240));

    ESP_LOGI(TAG, "Flashed, used %llu us", esp_timer_get_time() - ts);

    memset(buf, 0x00, 10240);
    swd_read_memory(0x08000000, buf, 10240);
    ESP_LOG_BUFFER_HEX(TAG, buf, 10240);

    ESP_LOGI(TAG, "Done!!");
}