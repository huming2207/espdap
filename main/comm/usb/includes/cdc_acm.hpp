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
#define SI_DEVICE_MODEL "Soul Injector ENG SAMPLE FOR SG"
#endif

#ifndef SI_DEVICE_BUILD
#define SI_DEVICE_BUILD "0.0.1"
#endif

namespace cdc_def
{
    enum file_recv_state : uint8_t {
        FILE_RECV_NONE = 0,
        FILE_RECV_FW = 1,
        FILE_RECV_ALGO = 2,
    };

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
        PKT_GET_ALGO_METADATA = 4,
        PKT_SET_ALGO_METADATA = 5,
        PKT_GET_FW_METADATA = 6,
        PKT_SET_FW_METADATA = 7,
        PKT_PING = 8,
        PKT_DATA_CHUNK = 9,
        PKT_CHUNK_ACK = 10,
        PKT_NACK = 0xff,
    };

    enum chunk_ack : uint8_t {
        CHUNK_XFER_DONE = 0,
        CHUNK_XFER_NEXT = 1,
        CHUNK_ERR_CRC32_FAIL = 2,
        CHUNK_ERR_UNEXPECTED = 3,
    };

    struct __attribute__((packed)) chunk_ack_pkt {
        chunk_ack state;

        // Can be:
        // 1. Next chunk index (when state == 1)
        // 2. Expected CRC32 (when state == 2)
        // 3. Max length allowed (when state == 3)
        // 4. Just 0 (when state == anything else?)
        uint32_t aux_info;
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

    struct __attribute__((packed)) fw_info {
        uint32_t crc; // 4
        uint32_t len; // 4
        char name[32]; // 32
    }; // 40 bytes
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
    static inline uint16_t get_crc16(const uint8_t *buf, size_t len, uint16_t init = 0x0000);

public:
    esp_err_t init();

private:
    void parse_pkt();
    void parse_get_config();
    void parse_set_config();
    void parse_get_algo_info();
    void parse_set_algo_metadata();
    void parse_get_fw_info();
    void parse_set_fw_metadata();
    void parse_chunk();

public:
    static esp_err_t send_ack(uint16_t crc = 0, uint32_t timeout_ms = portMAX_DELAY);
    static esp_err_t send_nack(uint32_t timeout_ms = portMAX_DELAY);
    static esp_err_t send_dev_info(uint32_t timeout_ms = portMAX_DELAY);
    static esp_err_t send_chunk_ack(cdc_def::chunk_ack state, uint32_t aux = 0, uint32_t timeout_ms = portMAX_DELAY);

private:
    static const constexpr char *TAG = "cdc_acm";
    EventGroupHandle_t rx_event = nullptr;
    volatile bool busy_decoding = false;
    volatile size_t curr_rx_len = 0;
    uint8_t *decoded_buf = nullptr;
    cdc_def::file_recv_state recv_state = cdc_def::FILE_RECV_NONE;
};

