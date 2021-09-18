#include <driver/spi_master.h>
#include <esp_log.h>

#include "swd_driver.hpp"

#define SWD_REG_START     (1u)
#define SWD_REG_AP        (1u << 1u)
#define SWD_REG_DP        (0u << 1u)
#define SWD_REG_READ      (1u << 2u)
#define SWD_REG_WRITE     (0u << 2u)
#define SWD_REG_ADDR(x)   ((x & 0x0Cu) << 3)
#define SWD_REG_STOP      (0u << 6u)
#define SWD_REG_PARK      (1u << 7u)
#define SWD_REG_BASE      (SWD_REG_START | SWD_REG_STOP | SWD_REG_PARK)

esp_err_t swd_driver::init(gpio_num_t swclk, gpio_num_t swdio, uint32_t freq_hz, spi_host_device_t host)
{
    spi_bus_config_t spi_bus_config = {};
    spi_bus_config.mosi_io_num     = swdio; // SWD I/O
    spi_bus_config.miso_io_num     = -1; // not connected
    spi_bus_config.sclk_io_num     = swclk; // SWD CLK
    spi_bus_config.quadwp_io_num   = -1;
    spi_bus_config.quadhd_io_num   = -1;
    spi_bus_config.max_transfer_sz = 0;
    spi_bus_config.flags = SPICOMMON_BUSFLAG_IOMUX_PINS;

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
    spi_dev->misc.ck_idle_edge = 0;
    spi_dev->user.ck_out_edge = 0;
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

void swd_driver::send_cycles(uint8_t clk_cycles, uint32_t swclk_level)
{
    spi_dev->user.usr_dummy = 0;
    spi_dev->user.usr_command = 0;
    spi_dev->user.usr_mosi = 1;
    spi_dev->user.usr_miso = 0;
    spi_dev->misc.ck_idle_edge = 0;
    spi_dev->user.usr_hold_pol = 1;
    spi_dev->user.usr_dout_hold = 0;
    spi_dev->user.ck_out_edge = 0;

    for (uint32_t idx = 0; idx < 18; idx++) {
        spi_dev->data_buf[idx] = swclk_level;
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

esp_err_t swd_driver::read_dp(uint8_t reg, uint32_t *out_word)
{
    uint8_t req = SWD_REG_BASE | SWD_REG_DP | SWD_REG_READ | SWD_REG_ADDR(reg) | calc_parity_req(reg);
    return recv_pkt(req, out_word);
}

uint8_t swd_driver::calc_parity_req(uint8_t reg)
{
    uint8_t cnt = 0;
    reg = reg >> 1;
    for (uint8_t idx = 0; idx < 4; idx++) {
        if ((reg & 0b1) == 1) cnt += 1;
        reg = reg >> 1;
    }

    return cnt % 2 == 0 ? 0 : 1;
}

uint32_t swd_driver::calc_parity_32(uint32_t reg)
{
    uint8_t cnt = 0;
    for (uint8_t idx = 0; idx < 32; idx++) {
        if ((reg & 0b1) == 1) cnt += 1;
        reg = reg >> 1;
    }

    return cnt % 2 == 0 ? 0 : 1;
}

esp_err_t swd_driver::recv_pkt(uint8_t reg, uint32_t *out_word, uint16_t retry_cnt)
{
//    bool ok = false;
//    while (!ok) {
        send_bits(reg, 8);
        wait_till_ready(100);

//        spi_dev->user.usr_dummy = 0;
//        spi_dev->user.usr_addr = 0;
//        spi_dev->user.usr_command = 0;
//        spi_dev->user.usr_mosi = 1;
//        spi_dev->user.usr_miso = 1;
//        spi_dev->user.ck_out_edge = 1;
//
//        spi_dev->miso_dlen.usr_miso_bit_len = 3; // 5+33 bits to read: two Trn bits and three return bits, 33 data bits
//        spi_dev->data_buf[0] = 0xff;
//        spi_dev->data_buf[1] = 0;
//        spi_dev->cmd.usr = 1; // Start command and then receive
//
//        while(spi_dev->cmd.usr != 0);
//        uint32_t ret = (spi_dev->data_buf[0] >> 1) & 0b111;
//        if (ret == 0b001) { // OK - done
//            *out_word = (spi_dev->data_buf[0] >> 5) | ((spi_dev->data_buf[1] & 0x7FFFFFFu) << 5);
//            return ESP_OK;
//        } else if (ret == 0b100) { // Fail, return (and reset?)
//            ESP_LOGE(TAG, "Fault detected, ret 0x%x", ret);
//            return ESP_FAIL;
//        } else {
//            continue;
//        }
//    }


    spi_transaction_t transaction = {};
    transaction.flags = SPI_TRANS_USE_RXDATA;
    transaction.rxlength = 32;
    spi_device_polling_transmit(spi_handle, &transaction);

    ESP_LOGI(TAG, "Got Rx: 0x%x 0x%x", transaction.rx_data[0], transaction.rx_data[1]);
    return ESP_OK;
}

esp_err_t IRAM_ATTR swd_driver::read_idcode(uint32_t *idcode)
{
//    send_cycles(8, 0);

    wait_till_ready(100);
    return read_dp(0, idcode);
}
