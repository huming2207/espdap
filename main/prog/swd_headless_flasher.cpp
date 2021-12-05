#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>
#include <led_ctrl.hpp>
#include <esp_crc.h>
#include <file_utils.hpp>
#include "swd_headless_flasher.hpp"

esp_err_t swd_headless_flasher::init()
{
    auto ret = led.init(GPIO_NUM_48);
    led.set_color(60,0,0,30);

    ret = ret ?: manifest.init();
    ret = ret ?: cfg_manager.init();

    ret = ret ?: file_utils::validate_firmware_file("/soul/firmware.bin", manifest.get_manifests()[0].fw_checksum);
    if (ret != ESP_OK) return ret;

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
            case flasher::VERIFY: {
                on_verify();
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
    uint32_t start_addr = 0, end_addr = 0;
    auto ret = cfg_manager.get_flash_start_addr(start_addr);
    ret = ret ?: cfg_manager.get_flash_end_addr(end_addr);
    ret = ret ?: swd.erase_sector(start_addr, end_addr);
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
    char path[128] = { 0 };
    snprintf(path, sizeof(path), "/soul/%s", manifest.get_manifests()[0].fw_name);
    int64_t ts = esp_timer_get_time();
    auto ret = swd.program_file(path, &written_len);
    if (ret != ESP_OK) {
        state = flasher::ERROR;
    } else {
        ts = esp_timer_get_time() - ts;
        double speed = written_len / ((double)ts / 1000000.0);
        ESP_LOGI(TAG, "Firmware written, len: %u, speed: %.2f bytes per sec", written_len, speed);
        state = flasher::VERIFY;
    }
}

void swd_headless_flasher::on_detect()
{
    ESP_LOGI(TAG, "Detecting");
    auto ret = swd.init(&cfg_manager);
    while (ret != ESP_OK) {
        on_error();
        ESP_LOGE(TAG, "Detect failed, retrying");
        ret = swd.init(&cfg_manager);
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

void swd_headless_flasher::on_verify()
{
    if (swd.verify(manifest.get_manifests()[0].fw_checksum, UINT32_MAX, written_len) != ESP_OK) {
        state = flasher::ERROR;
    } else {
        state = flasher::DONE;
    }
}
