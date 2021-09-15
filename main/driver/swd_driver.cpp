#include <driver/spi_master.h>
#include <esp_log.h>

#include "swd_driver.hpp"

esp_err_t swd_driver::init(gpio_num_t swclk, gpio_num_t swdio, uint32_t freq_hz, spi_host_device_t host)
{
    spi_bus_config_t spi_bus_config = {};
    spi_bus_config.mosi_io_num     = swdio; // SWD I/O
    spi_bus_config.miso_io_num     = -1; // not connected
    spi_bus_config.sclk_io_num     = swclk; // SWD CLK
    spi_bus_config.quadwp_io_num   = -1;
    spi_bus_config.quadhd_io_num   = -1;
    spi_bus_config.max_transfer_sz = 0;

    auto ret = spi_bus_initialize(host, &spi_bus_config, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI Bus init fail");
        return ret;
    }


    spi_device_interface_config_t spi_dev_inf_config = {};
    spi_dev_inf_config.command_bits        = 0;
    spi_dev_inf_config.address_bits        = 0;
    spi_dev_inf_config.dummy_bits          = 0;
    spi_dev_inf_config.mode                = 0;
    spi_dev_inf_config.duty_cycle_pos      = 0;
    spi_dev_inf_config.cs_ena_pretrans     = 0;
    spi_dev_inf_config.cs_ena_posttrans    = 0;
    spi_dev_inf_config.clock_speed_hz      = (int)freq_hz;
    spi_dev_inf_config.spics_io_num        = -1;
    spi_dev_inf_config.flags               = SPI_DEVICE_3WIRE | SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_BIT_LSBFIRST;
    spi_dev_inf_config.queue_size          = 24;
    spi_dev_inf_config.pre_cb              = nullptr;
    spi_dev_inf_config.post_cb             = nullptr;

    ret = spi_bus_add_device(host, &spi_dev_inf_config, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI device init fail");
        return ret;
    }


    ret = spi_device_acquire_bus(spi_handle, portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI lock fail");
        return ret;
    }

    switch (host) {
        case SPI1_HOST: {
            spi_dev = (volatile spi_dev_t *)(DR_REG_SPI1_BASE);
            break;
        }

        case SPI2_HOST: {
            spi_dev = (volatile spi_dev_t *)(DR_REG_SPI2_BASE);
            break;
        }

        case SPI3_HOST: {
            spi_dev = (volatile spi_dev_t *)(DR_REG_SPI3_BASE);
            break;
        }
    }

    return ret;
}

void swd_driver::send_bits(uint32_t bits, size_t bit_len)
{
    spi_dev->user.usr_dummy = 0;
    spi_dev->user.usr_command = 0;
    spi_dev->user.usr_mosi = 1;
    spi_dev->user.usr_miso = 0;
    spi_dev->mosi_dlen.usr_mosi_bit_len = bit_len - 1;
    spi_dev->data_buf[0] = bits;
    spi_dev->cmd.usr = 1; // Trigger Tx!!
}

esp_err_t swd_driver::wait_till_ready(int32_t timeout_cycles)
{
    while(timeout_cycles >= 0 && spi_dev->cmd.usr != 0) {
        timeout_cycles--;
    }

    if (timeout_cycles < 0) {
        return ESP_ERR_TIMEOUT;
    } else {
        return ESP_OK;
    }
}

void swd_driver::send_cycles(uint8_t clk_cycles)
{
    spi_dev->user.usr_dummy = 0;
    spi_dev->user.usr_command = 0;
    spi_dev->user.usr_mosi = 1;
    spi_dev->user.usr_miso = 0;

    for (uint32_t idx = 0; idx < 18; idx++) {
        spi_dev->data_buf[idx] = 0xffffffff;
    }

    spi_dev->mosi_dlen.usr_mosi_bit_len = clk_cycles - 1;
    spi_dev->miso_dlen.usr_miso_bit_len = 0;
    spi_dev->cmd.usr = 1; // Trigger Tx!!
}

void swd_driver::send_swj_sequence()
{
    const uint16_t seq = 0xE79E;
    send_cycles(51);
    while(spi_dev->cmd.usr != 0);
    send_bits(seq, 16);
    while(spi_dev->cmd.usr != 0);
    send_cycles(51);
    while(spi_dev->cmd.usr != 0);
}
