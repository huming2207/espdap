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

    ret = lvgl_take_lock(pdMS_TO_TICKS(1000));

    lv_obj_t *top_bar = nullptr;
    lv_obj_t *bottom_bar = nullptr;
    ret = ret ?: draw_two_bars(&top_bar, &bottom_bar, lv_color_hex(0x27632a), lv_color_hex(0xff9800));

    auto *top_text = lv_label_create(top_bar);
    lv_obj_align(top_text, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_align(top_text, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(top_text, "Soul Injector\nFirmware Programmer");
    lv_obj_set_style_text_font(top_text, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(top_text, lv_color_white(), 0);

    auto *bottom_text = lv_label_create(bottom_bar);
    lv_obj_align(bottom_text, LV_ALIGN_TOP_LEFT, 10, 2);
    lv_obj_set_style_text_align(bottom_text, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_text_fmt(bottom_text, "By Jackson M Hu\nCopyright (C) 2023\nNO COMMERCIAL USE\nUNLESS APPROVED BY AUTHOR\n\nModel: %s\nVersion: %s\nSDK: %s", CONFIG_SI_DEVICE_MODEL, CONFIG_SI_DEVICE_BUILD, IDF_VER);
    lv_obj_set_style_text_font(bottom_text, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(bottom_text, lv_color_black(), 0);

    lvgl_give_lock();
    return ret;
}

esp_err_t lcd_manager::display_detect()
{
    auto ret = clear_display();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = lvgl_take_lock(pdMS_TO_TICKS(1000));

    lv_obj_t *top_bar = nullptr;
    lv_obj_t *bottom_bar = nullptr;
    ret = ret ?: draw_two_bars(&top_bar, &bottom_bar, lv_color_black(), lv_color_hex(0x006065)); // Dark cyan

    lvgl_give_lock();
    return ret;
}

esp_err_t lcd_manager::display_erase(bool is_chip_erase)
{
    auto ret = clear_display();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = lvgl_take_lock(pdMS_TO_TICKS(1000));

    lv_obj_t *top_bar = nullptr;
    lv_obj_t *bottom_bar = nullptr;
    ret = ret ?: draw_two_bars(&top_bar, &bottom_bar, lv_color_black(), lv_color_hex(0x01579b)); // Dark blue

    lvgl_give_lock();
    return ret;
}

esp_err_t lcd_manager::display_program(uint8_t percentage)
{
    auto ret = clear_display();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = lvgl_take_lock(pdMS_TO_TICKS(1000));

    lv_obj_t *top_bar = nullptr;
    lv_obj_t *bottom_bar = nullptr;
    ret = ret ?: draw_two_bars(&top_bar, &bottom_bar, lv_color_black(), lv_color_hex(0xffab00)); // Orange

    lvgl_give_lock();
    return ret;
}

esp_err_t lcd_manager::display_error(const char *err_heading, const char *err_msg)
{
    auto ret = clear_display();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = lvgl_take_lock(pdMS_TO_TICKS(1000));

    lv_obj_t *top_bar = nullptr;
    lv_obj_t *bottom_bar = nullptr;
    ret = ret ?: draw_two_bars(&top_bar, &bottom_bar, lv_color_black(), lv_color_hex(0xff0000)); // Red

    lvgl_give_lock();
    return ret;
}

esp_err_t lcd_manager::display_verify(const char *verify_msg)
{
    auto ret = clear_display();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = lvgl_take_lock(pdMS_TO_TICKS(1000));

    lv_obj_t *top_bar = nullptr;
    lv_obj_t *bottom_bar = nullptr;
    ret = ret ?: draw_two_bars(&top_bar, &bottom_bar, lv_color_black(), lv_color_hex(0x512da8)); // Red

    lvgl_give_lock();
    return ret;
}

esp_err_t lcd_manager::display_done()
{
    auto ret = clear_display();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = lvgl_take_lock(pdMS_TO_TICKS(1000));

    lv_obj_t *top_bar = nullptr;
    lv_obj_t *bottom_bar = nullptr;
    ret = ret ?: draw_two_bars(&top_bar, &bottom_bar, lv_color_black(), lv_color_hex(0x1b5e20)); // Red

    lvgl_give_lock();
    return ret;
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
    lv_obj_set_align(root_obj, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(root_obj, 0, 0);
    lv_obj_set_style_radius(root_obj, 0, 0);
    lv_obj_set_scrollbar_mode(root_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(root_obj, 0, 0);
    lv_obj_set_style_border_width(root_obj, 0, 0);

    lvgl_give_lock();
    return ESP_OK;
}

esp_err_t lcd_manager::draw_two_bars(lv_obj_t **top_out, lv_obj_t **bottom_out, lv_color_t top_color, lv_color_t bottom_color)
{
    if (top_out == nullptr || bottom_out == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    if (root_obj == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    auto *top_obj = lv_obj_create(root_obj);
    if (top_obj == nullptr) {
        return ESP_FAIL;
    }

    lv_obj_set_size(top_obj, 240, 120);
    lv_obj_set_align(top_obj, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(top_obj, 0, 0);
    lv_obj_set_style_radius(top_obj, 0, 0);
    lv_obj_set_style_bg_color(top_obj, top_color, 0);
    lv_obj_set_scrollbar_mode(top_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(top_obj, 0, 0);
    lv_obj_set_style_border_width(top_obj, 0, 0);
    *top_out = top_obj;

    auto *bottom_obj = lv_obj_create(root_obj);
    if (bottom_obj == nullptr) {
        return ESP_FAIL;
    }

    lv_obj_set_size(bottom_obj, 240, 120);
    lv_obj_set_align(bottom_obj, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(bottom_obj, 0, 120);
    lv_obj_set_style_radius(bottom_obj, 0, 0);
    lv_obj_set_style_bg_color(bottom_obj, bottom_color, 0); // Dark cyan (need white text)
    lv_obj_set_scrollbar_mode(bottom_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(bottom_obj, 0, 0);
    lv_obj_set_style_border_width(bottom_obj, 0, 0);
    *bottom_out = bottom_obj;

    return ESP_OK;
}
