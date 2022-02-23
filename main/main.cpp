#include <cstring>
#include <esp_log.h>
#include <swd_headless_flasher.hpp>
#include <pthread.h>
#include "wasm_manager.hpp"

extern const uint8_t wasm_blob_start[] asm("_binary_wasi_wasm_start");
extern const uint8_t wasm_blob_end[] asm("_binary_wasi_wasm_end");

static void *test_main(void *arg)
{
    auto &wasm = wasm_manager::instance();
    ESP_ERROR_CHECK(wasm.init());
    ESP_ERROR_CHECK(wasm.load(wasm_blob_start, wasm_blob_end - wasm_blob_start));
    return nullptr;
}

extern "C" void app_main(void)
{
    static const char *TAG = "main";
    ESP_LOGI(TAG, "Started");

//    auto &ble_serv = ble_serv_mgr::instance();
//    ESP_ERROR_CHECK(ble_serv.init());
//
//    auto &flasher = swd_headless_flasher::instance();
//    flasher.init();

    pthread_t t;
    pthread_attr_t tattr;
    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setstacksize(&tattr, 32768);

    int res = pthread_create(&t, &tattr, test_main, (void *) nullptr);
    assert(res == 0);

    res = pthread_join(t, nullptr);
    assert(res == 0);

    ESP_LOGI(TAG, "Exiting...");

}