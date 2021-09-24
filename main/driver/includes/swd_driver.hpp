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
    SWD_FAST_FUNC_ATTR void send_cycles(uint8_t clk_cycles, uint32_t swclk_level = 0xffffffff);
    SWD_FAST_FUNC_ATTR esp_err_t wait_till_ready(int32_t timeout_cycles);
    IRAM_ATTR esp_err_t recv_pkt(uint8_t reg, uint32_t *out_word, uint16_t retry_cnt = 1000);
    SWD_FAST_FUNC_ATTR esp_err_t read_dp(uint8_t reg, uint32_t *out_word);
    static SWD_FAST_FUNC_ATTR uint8_t calc_parity_req(uint8_t reg);
    static SWD_FAST_FUNC_ATTR uint32_t calc_parity_32(uint32_t reg);

public:
    esp_err_t init(gpio_num_t swclk = GPIO_NUM_1, gpio_num_t swdio = GPIO_NUM_2, uint32_t freq_hz = 5000000, spi_host_device_t host = SPI2_HOST);
    void IRAM_ATTR send_swj_sequence();
    esp_err_t IRAM_ATTR read_idcode(uint32_t *idcode);
};

