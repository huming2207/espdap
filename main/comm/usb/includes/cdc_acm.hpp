#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include <tinyusb.h>
#include <tusb_cdc_acm.h>
#include <esp_err.h>

#define SLIP_END            0xc0
#define SLIP_ESC            0xdb
#define SLIP_ESC_END        0xdc
#define SLIP_ESC_ESC        0xdd

#ifndef SI_DEVICE_MODEL
#define SI_DEVICE_MODEL "SI R&D"
#endif

#ifndef SI_DEVICE_BUILD
#define SI_DEVICE_BUILD "0.0.1"
#endif

namespace cdc_def
{
    enum event : uint32_t {
        EVT_NEW_PACKET = BIT(0),
        EVT_READING_PKT = BIT(1),
        EVT_SLIP_ERROR = BIT(2),
    };

    enum pkt_type : uint8_t {
        PKT_ACK = 0,
        PKT_DEVICE_INFO = 1,
        PKT_GET_CONFIG = 2,
        PKT_SET_CONFIG = 3,
        PKT_GET_ALGO_INFO = 4,
        PKT_SET_ALGO_BIN = 5,
        PKT_GET_FW_INFO = 6,
        PKT_SET_FW_BIN = 7,
        PKT_NACK = 0xff,
    };

    struct __attribute__((packed)) header {
        pkt_type type;
        uint8_t len;
        uint16_t crc;
    };

    struct __attribute__((packed)) ack_pkt {
        pkt_type type;
        uint8_t len;
        uint16_t crc;
    };

    struct __attribute__((packed)) device_info {
        uint8_t mac_addr[6];
        uint8_t flash_id[8];
        char esp_idf_ver[32];
        char dev_model[32];
        char dev_build[32];
    };
}

class cdc_acm
{
public:
    static cdc_acm& instance()
    {
        static cdc_acm instance;
        return instance;
    }

    cdc_acm(cdc_acm const &) = delete;
    void operator=(cdc_acm const &) = delete;

private:
    cdc_acm() = default;
    static void serial_rx_cb(int itf, cdcacm_event_t *event);
    [[noreturn]] static void rx_handler_task(void *ctx);
    static esp_err_t send_pkt(cdc_def::pkt_type type, const uint8_t *buf, size_t len, uint32_t timeout_ms = portMAX_DELAY);
    static esp_err_t encode_and_tx(const uint8_t *header_buf, size_t header_len, const uint8_t *buf, size_t len, uint32_t timeout_ms = portMAX_DELAY);
    static inline uint16_t get_crc16_ccitt(const uint8_t *buf, size_t len, uint16_t init = 0xffff);

public:
    esp_err_t init();

private:
    void parse_pkt();
    void parse_get_config();
    void parse_set_config();
    void parse_get_algo_info();
    void parse_set_algo_bin();
    void parse_get_fw_info();
    void parse_set_fw_bin();

public:
    static esp_err_t send_ack(uint16_t crc = 0, uint32_t timeout_ms = portMAX_DELAY);
    static esp_err_t send_nack(uint32_t timeout_ms = portMAX_DELAY);
    static esp_err_t send_dev_info(uint32_t timeout_ms = portMAX_DELAY);

private:
    static const constexpr char *TAG = "cdc_acm";
    static const constexpr uint8_t slip_esc_end[] = { SLIP_ESC, SLIP_ESC_END };
    static const constexpr uint8_t slip_esc_esc[] = { SLIP_ESC, SLIP_ESC_ESC };
    EventGroupHandle_t rx_event = nullptr;
    volatile bool busy_decoding = false;
    volatile size_t curr_rx_len = 0;
    uint8_t *decoded_buf = nullptr;
};

