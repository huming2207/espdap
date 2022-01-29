#include <cstring>
#include <esp_log.h>
#include <swd_headless_flasher.hpp>

extern "C" void app_main(void)
{
    static const char *TAG = "main";
    ESP_LOGI(TAG, "Started");

//    auto &ble_serv = ble_serv_mgr::instance();
//    ESP_ERROR_CHECK(ble_serv.init());
//
    auto &flasher = swd_headless_flasher::instance();
    flasher.init();
}