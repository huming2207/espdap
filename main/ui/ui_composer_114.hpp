#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <lvgl.h>
#include "ui_commander.hpp"
#include "ui/ui_if.hpp"

class ui_composer_114 : public ui_composer_sm
{
public:
    friend class display_manager;
    esp_err_t init() override;

private:
    // These functions below should be called in UI thread only
    esp_err_t draw_init(ui_state::queue_item *screen)  override;
    esp_err_t draw_erase(ui_state::queue_item *screen) override;
    esp_err_t draw_flash(ui_state::queue_item *screen) override;
    esp_err_t draw_test(ui_state::queue_item *screen)  override;
    esp_err_t draw_error(ui_state::queue_item *screen) override;
    esp_err_t draw_done(ui_state::queue_item *screen) override;
    esp_err_t draw_usb(ui_state::queue_item *screen) override;
    esp_err_t draw_anything(ui_state::queue_item *screen) override;

    esp_err_t wait_and_draw();

private:
    esp_err_t recreate_widget(bool with_comment = false, bool with_qrcode = false, lv_color_t dark_color = lv_color_black(), lv_color_t bright_color = lv_color_white());

private:
    ui_state::display_state curr_state = ui_state::STATE_EMPTY;
    lv_obj_t *disp_obj = nullptr;
    lv_obj_t *base_obj = nullptr;
    lv_obj_t *top_sect = nullptr;
    lv_obj_t *bottom_sect = nullptr;
    lv_obj_t *top_label = nullptr;
    lv_obj_t *bottom_label = nullptr;
    lv_obj_t *bottom_comment = nullptr;

private:
    static const constexpr char TAG[] = "ui_114";
};