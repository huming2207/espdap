#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "ui/ui_if.hpp"

class ui_commander
{
public:
    static ui_commander *instance()
    {
        static ui_commander _instance;
        return &_instance;
    }

    ui_commander(ui_commander const &) = delete;
    void operator=(ui_commander const &) = delete;

public:
    esp_err_t init();

public:
    esp_err_t display_init();
    esp_err_t display_chip_erase();
    esp_err_t display_flash(ui_state::flash_screen *screen);
    esp_err_t display_test(ui_state::test_screen *screen);
    esp_err_t display_error(ui_state::error_screen *screen);
    esp_err_t display_done();
    esp_err_t display_usb();
    esp_err_t display_anything(ui_state::queue_item *item);
    esp_err_t display_error(esp_err_t ret, const char *qrcode, const char *fmt, ...);

private:
    QueueHandle_t task_queue = nullptr;
    ui_commander() = default;

private:
    static const constexpr char TAG[] = "ui_cmder";
};
