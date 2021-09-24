#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/rmt.h>

class led_ctrl
{
public:
    static led_ctrl& instance()
    {
        static led_ctrl instance;
        return instance;
    }

    led_ctrl(manifest_mgr const &) = delete;
    void operator=(led_ctrl const &) = delete;

private:
    led_ctrl() = default;

public:
    esp_err_t init(gpio_num_t pin = GPIO_NUM_18) const
    {
        rmt_config_t config;
        config.rmt_mode = RMT_MODE_TX;
        config.channel = RMT_CHANNEL_0;
        config.gpio_num = pin;
        config.mem_block_num = 3;
        config.tx_config.loop_en = false;
        config.tx_config.carrier_en = false;
        config.tx_config.idle_output_en = true;
        config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
        config.clk_div = 2;

        auto ret = rmt_config(&config);
        ret = ret ?: rmt_driver_install(config.channel, 0, 0);
        return ret;
    }

    void set_color(uint8_t r, uint8_t g, uint8_t b, uint32_t wait_ms)
    {
        rmt_item32_t led_data_buffer[24] = {};
        uint32_t bits_to_send = ((r << 16u) | (g << 8u) | b);
        uint32_t mask = 1 << (24 - 1);
        for (uint32_t bit = 0; bit < 24; bit++) {
            uint32_t bit_is_set = bits_to_send & mask;
            led_data_buffer[bit] = bit_is_set ?
                    (rmt_item32_t){{{52, 1, 52, 0}}} :
                    (rmt_item32_t){{{14, 1, 52, 0}}};
            mask >>= 1;
        }

        ESP_ERROR_CHECK(rmt_write_items(RMT_CHANNEL_0, led_data_buffer, 24, false));
        if (wait_ms > 0) {
            ESP_ERROR_CHECK(rmt_wait_tx_done(RMT_CHANNEL_0, pdMS_TO_TICKS(wait_ms)));
        }
    }
};

