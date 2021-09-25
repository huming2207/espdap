#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>

#include <flash_algo.hpp>
#include <led_ctrl.hpp>
#include <manifest_mgr.hpp>
#include "swd_headless_flasher.hpp"

esp_err_t swd_headless_flasher::init()
{
    auto ret = led.init(GPIO_NUM_18);
    led.set_color(60,0,0,30);

    auto &manifest = manifest_mgr::instance();
    ret = ret ?: manifest.init();
    ret = ret ?: algo.init("/soul/flash_algo.json");

    while (true) {
        switch (state) {
            case flasher::DETECT: {
                on_detect();
                break;
            }

            case flasher::ERASE: {
                on_erase();
                break;
            }

            case flasher::PROGRAM: {
                on_program();
                break;
            }

            case flasher::ERROR: {
                on_error();
                break;
            }
            case flasher::DONE: {
                on_done();
                break;
            }
        }
    }

    return ret;
}

void swd_headless_flasher::on_error()
{
    led.set_color(80, 0, 0, 50);
    vTaskDelay(pdMS_TO_TICKS(100));
    led.set_color(0, 0, 0, 50);
    vTaskDelay(pdMS_TO_TICKS(100));
}

void swd_headless_flasher::on_erase()
{
    ESP_LOGI(TAG, "Erasing");
    auto ret = swd.erase_sector(algo.get_flash_start_addr(), algo.get_flash_end_addr());
    if (ret != ESP_OK) {
        for (uint32_t idx = 0; idx < 32; idx++) {
            on_error();
        }

        state = flasher::DETECT;
    }

    state = flasher::PROGRAM;
}

void swd_headless_flasher::on_program()
{
    state = flasher::DONE;
}

void swd_headless_flasher::on_detect()
{
    ESP_LOGI(TAG, "Detecting");
    auto ret = swd.init(&algo);
    while (ret != ESP_OK) {
        on_error();
        ESP_LOGE(TAG, "Detect failed, retrying");
        ret = swd.init(&algo);
    }

    state = flasher::ERASE; // To erase
}

void swd_headless_flasher::on_done()
{
    led.set_color(0, 80, 0, 50);
    vTaskDelay(pdMS_TO_TICKS(50));
    led.set_color(0, 0, 0, 50);
    vTaskDelay(pdMS_TO_TICKS(200));
}
