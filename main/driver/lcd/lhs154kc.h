#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <esp_err.h>
#include <lvgl.h>

#define ST7789_CMD 0
#define ST7789_DAT 1

#define ST7789_DISP_BUF_SIZE (240 * 40)
#define CONFIG_LCD_IO_DC 15
#define CONFIG_LCD_SPI_CS 7
#define CONFIG_LCD_SPI_SCLK 6
#define CONFIG_LCD_SPI_MOSI 5
#define CONFIG_LCD_IO_RST 4

typedef struct {
    uint8_t reg;
    uint8_t data[6];
    uint8_t len;
} st7789_seq_t;

esp_err_t lv_st7789_init();
void lv_st7789_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t color);
void lv_st7789_flush(lv_disp_drv_t *disp_drv, const lv_area_t * area, lv_color_t * color_p);

#ifdef __cplusplus
}
#endif
