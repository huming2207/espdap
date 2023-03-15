#include <esp_err.h>
#include "lcd_manager.hpp"
#include "lvgl_wrapper.h"

esp_err_t lcd_manager::init()
{
    auto ret = lvgl_disp_init();

    if (ret == ESP_OK) {
        curr_state = lcd::STATE_SPLASH;
        return display_splash();
    }

    return ret;
}

esp_err_t lcd_manager::display_splash()
{
    auto ret = clear_display();
    if (ret != ESP_OK) {
        return ret;
    }

    auto *title_label = lv_label_create(root_obj);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_14, 0);
    lv_label_set_text_fmt(title_label, "Soul Injector\nFirmware Programmer\n\nBy Jackson M Hu\nModel: %s\nVersion: %s\nSDK: %s", CONFIG_SI_DEVICE_MODEL, CONFIG_SI_DEVICE_BUILD, IDF_VER);


    return ret;
}

esp_err_t lcd_manager::display_detect()
{
    return 0;
}

esp_err_t lcd_manager::display_erase(bool is_chip_erase)
{
    return 0;
}

esp_err_t lcd_manager::display_program(uint8_t percentage)
{
    return 0;
}

esp_err_t lcd_manager::display_error(const char *err_heading, const char *err_msg)
{
    return 0;
}

esp_err_t lcd_manager::display_verify(const char *verify_msg)
{
    return 0;
}

esp_err_t lcd_manager::display_done()
{
    return 0;
}

esp_err_t lcd_manager::clear_display()
{
    auto ret = lvgl_take_lock(pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        return ret;
    }

    if (root_obj != nullptr) {
        lv_obj_del(root_obj);
    }

    root_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(root_obj, 240, 240);

    lvgl_give_lock();
    return ESP_OK;
}
