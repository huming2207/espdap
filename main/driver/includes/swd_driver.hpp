#pragma once

#include <esp_err.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>

#define SWD_FAST_FUNC_ATTR __always_inline   IRAM_ATTR

class swd_driver
{
public:
    static swd_driver& instance()
    {
        static swd_driver instance;
        return instance;
    }

    swd_driver(swd_driver const &) = delete;
    void operator=(swd_driver const &) = delete;

private:
    static const constexpr char *TAG = "swd_drv";
    spi_device_handle_t spi_handle = nullptr;
    volatile spi_dev_t *spi_dev = nullptr;

private:
    swd_driver() = default;
    SWD_FAST_FUNC_ATTR void send_bits(uint32_t bits, size_t bit_len);
    SWD_FAST_FUNC_ATTR void send_cycles(uint8_t clk_cycles);
    SWD_FAST_FUNC_ATTR esp_err_t wait_till_ready(int32_t timeout_cycles);

public:
    esp_err_t init(gpio_num_t swclk = GPIO_NUM_1, gpio_num_t swdio = GPIO_NUM_2, uint32_t freq_hz = 10000000, spi_host_device_t host = SPI3_HOST);
    void send_swj_sequence();
};

