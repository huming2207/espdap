#include <esp_log.h>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>

#include "lvgl_wrapper.h"
#include "lhs154kc.h"

#ifdef TAG
#undef TAG
#endif

#define TAG "cal_disp"

#ifdef LV_TICK_PERIOD_MS
#undef LV_TICK_PERIOD_MS
#endif

#define LV_TICK_PERIOD_MS 1

static uint8_t *buf_a = NULL;
static uint8_t *buf_b = NULL;
static lv_disp_draw_buf_t draw_buf = {};
static esp_timer_handle_t timer_handle = NULL;



static void IRAM_ATTR lv_tick_cb(void *arg)
{
    (void) arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

esp_err_t cal_disp_init()
{
    ESP_LOGI(TAG, "LVGL init");
    lv_init();

    ESP_LOGI(TAG, "Display hardware init");
    lv_st7789_init();

    buf_a = heap_caps_malloc(ST7789_DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    if (buf_a == NULL) {
        ESP_LOGE(TAG, "Failed to allocate display buffer");
        return ESP_ERR_NO_MEM;
    }

    buf_b = heap_caps_malloc(ST7789_DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    if (buf_b == NULL) {
        ESP_LOGE(TAG, "Failed to allocate display buffer");
        free(buf_a);
        return ESP_ERR_NO_MEM;
    }

    lv_disp_draw_buf_init(&draw_buf, buf_a, buf_b, ST7789_DISP_BUF_SIZE);
    static lv_disp_drv_t disp_drv = {};
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = lv_st7789_flush;
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 240;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.antialiasing = 1;

    lv_disp_drv_register(&disp_drv);

    esp_timer_create_args_t timer_args = {
            .name = "lvgl_timer",
            .callback = lv_tick_cb,
    };

    esp_err_t ret = esp_timer_create(&timer_args, &timer_handle);
    ret = ret ?: esp_timer_start_periodic(timer_handle, LV_TICK_PERIOD_MS * 1000);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set up timer");
        free(buf_a);
        free(buf_b);
    } else {
        ESP_LOGI(TAG, "Display init done");
    }

    return ret;
}