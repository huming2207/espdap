#pragma once

#include <config_manager.hpp>
#include <led_ctrl.hpp>
#include <manifest_mgr.hpp>
#include <esp_err.h>
#include "swd_prog.hpp"

namespace flasher
{
    enum pg_state
    {
        DETECT = 0,
        ERASE = 1,
        PROGRAM = 2,
        ERROR = 3,
        VERIFY = 4,
        DONE = 5,
    };
}

class swd_headless_flasher
{
public:
    static swd_headless_flasher& instance()
    {
        static swd_headless_flasher instance;
        return instance;
    }

    swd_headless_flasher(swd_headless_flasher const &) = delete;
    void operator=(swd_headless_flasher const &) = delete;

private:
    swd_headless_flasher() = default;
    uint32_t written_len = 0;
    config_manager &cfg_manager = config_manager::instance();
    led_ctrl &led = led_ctrl::instance();
    swd_prog &swd = swd_prog::instance();
    manifest_mgr &manifest = manifest_mgr::instance();
    flasher::pg_state state = flasher::DETECT;

    static const constexpr char *TAG = "swd_hdls_flr";

public:
    esp_err_t init();

private:
    void on_detect();
    void on_error();
    void on_erase();
    void on_program();
    void on_verify();
    void on_done();
};

