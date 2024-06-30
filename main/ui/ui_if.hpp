#pragma once

#include <esp_err.h>
#include <esp_log.h>
#include <lvgl.h>

namespace ui_state
{
    struct init_screen
    {
        char subtitle[32];
    };

    struct erase_screen
    {
        uint8_t percentage;
        char subtitle[32];
    };

    struct flash_screen
    {
        uint8_t percentage;
        char subtitle[32];
    };

    struct test_screen
    {
        uint8_t done_test;
        uint8_t total_test;
        char subtitle[32];
    };

    struct error_screen
    {
        esp_err_t ret;
        char comment[32];
        char qrcode[64];
    };

    enum display_state : int8_t
    {
        STATE_EMPTY = -1,
        STATE_INIT = 0,
        STATE_ERASE = 1,
        STATE_FLASH = 2,
        STATE_TEST  = 3,
        STATE_ERROR = 4,
        STATE_DONE = 5,
        STATE_USB = 6,
    };

    struct __attribute__((packed)) queue_item
    {
        display_state state;
        uint8_t percentage;
        uint8_t total_count;
        lv_color_t bg_color;
        char title[16];
        char comment[32];
        char qrcode[64];
    };
};

class ui_composer_sm
{
public:
    virtual esp_err_t init() = 0;

public:
    // These functions below should be called in UI thread only
    virtual esp_err_t draw_init(ui_state::queue_item *screen)  = 0;
    virtual esp_err_t draw_erase(ui_state::queue_item *screen) = 0;
    virtual esp_err_t draw_flash(ui_state::queue_item *screen) = 0;
    virtual esp_err_t draw_test(ui_state::queue_item *screen)  = 0;
    virtual esp_err_t draw_error(ui_state::queue_item *screen) = 0;
    virtual esp_err_t draw_done(ui_state::queue_item *screen) = 0;
    virtual esp_err_t draw_usb(ui_state::queue_item *screen) = 0;
    virtual esp_err_t draw_anything(ui_state::queue_item *screen) = 0;
};