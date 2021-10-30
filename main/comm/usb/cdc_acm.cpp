#include <esp_log.h>
#include <esp_crc.h>
#include <esp_flash.h>

#include "cdc_acm.hpp"



esp_err_t cdc_acm::init()
{
    tinyusb_config_t tusb_cfg = {}; // the configuration using default values
    auto ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TinyUSB driver install failed");
        return ret;
    }

    tinyusb_config_cdcacm_t acm_cfg = {};
    acm_cfg.usb_dev = TINYUSB_USBDEV_0;
    acm_cfg.cdc_port = TINYUSB_CDC_ACM_0;
    acm_cfg.rx_unread_buf_sz = 64;
    acm_cfg.callback_rx = &serial_rx_cb;
    acm_cfg.callback_rx_wanted_char = nullptr;
    acm_cfg.callback_line_state_changed = nullptr;
    acm_cfg.callback_line_coding_changed = nullptr;

    ret = ret ?: tusb_cdc_acm_init(&acm_cfg);

    rx_event = xEventGroupCreate();
    if (rx_event == nullptr) {
        ESP_LOGE(TAG, "Failed to create Rx event group");
        return ESP_ERR_NO_MEM;
    }

    decoded_buf = static_cast<uint8_t *>(heap_caps_malloc(CDC_ACM_DECODED_BUF_LEN, MALLOC_CAP_SPIRAM));
    if (decoded_buf == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate SLIP decode buf");
        return ESP_ERR_NO_MEM;
    }

    return ret;
}

void cdc_acm::serial_rx_cb(int itf, cdcacm_event_t *event)
{
    auto &ctx = cdc_acm::instance();

    uint8_t rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE] = { 0 };
    size_t rx_size = 0;
    auto ret = tinyusb_cdcacm_read(static_cast<tinyusb_cdcacm_itf_t>(itf), rx_buf, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TinyUSB read fail!");
        return;
    }

    if (rx_size < 1) return;

    size_t idx = 0;
    while (idx < rx_size && ctx.curr_rx_len < CDC_ACM_DECODED_BUF_LEN) {
        if (rx_buf[idx] == SLIP_END) {
            if (ctx.curr_rx_len > 0) {
                xEventGroupSetBits(ctx.rx_event, cdc_def::EVT_NEW_PACKET);
            } else {
                xEventGroupClearBits(ctx.rx_event, cdc_def::EVT_NEW_PACKET);
            }
        } else if (rx_buf[idx] == SLIP_ESC) {
            idx += 1;
            if (rx_buf[idx] == SLIP_ESC_END) {
                ctx.decoded_buf[ctx.curr_rx_len] = SLIP_END;
            } else if (rx_buf[idx] == SLIP_ESC_ESC) {
                ctx.decoded_buf[ctx.curr_rx_len] = SLIP_ESC;
            } else {
                xEventGroupSetBits(ctx.rx_event, cdc_def::EVT_SLIP_ERROR);
                ESP_LOGE(TAG, "SLIP decoding detected a corrupted packet");
                return;
            }
        } else {
            ctx.decoded_buf[ctx.curr_rx_len] = rx_buf[idx];
        }

        ctx.curr_rx_len += 1;
    }
}

[[noreturn]] void cdc_acm::rx_handler_task(void *_ctx)
{
    auto &ctx = cdc_acm::instance();
    while(true) {
        if (xEventGroupWaitBits(ctx.rx_event, cdc_def::EVT_NEW_PACKET, pdTRUE, pdFALSE, portMAX_DELAY) == pdTRUE) {

        }


    }
}

esp_err_t cdc_acm::send_ack(uint16_t crc, uint32_t timeout_ms)
{
    cdc_def::ack_pkt ack = {};
    ack.crc = crc;
    ack.type = cdc_def::PKT_ACK;
    ack.len = 0;

    auto sent_len = tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, (uint8_t *)&ack, sizeof(cdc_def::ack_pkt));
    if (sent_len < sizeof(cdc_def::ack_pkt)) {
        ESP_LOGE(TAG, "Failed to send ACK");
        return ESP_FAIL;
    } else {
        return tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, pdMS_TO_TICKS(timeout_ms));
    }
}

esp_err_t cdc_acm::send_dev_info(uint32_t timeout_ms)
{
    const char *idf_ver = IDF_VER;
    const char *dev_model = SI_DEVICE_MODEL;
    const char *dev_build = SI_DEVICE_BUILD;

    cdc_def::device_info dev_info = {};
    auto ret = esp_efuse_mac_get_default(dev_info.mac_addr);
    ret = ret ?: esp_flash_read_unique_chip_id(esp_flash_default_chip, (uint64_t *)dev_info.flash_id);
    strcpy(dev_info.esp_idf_ver, idf_ver);
    strcpy(dev_info.dev_build, dev_build);
    strcpy(dev_info.dev_model, dev_model);

    ret = ret ?: send_pkt(cdc_def::PKT_DEVICE_INFO, (uint8_t *)&dev_info, sizeof(dev_info), timeout_ms);

    return ret;
}

esp_err_t cdc_acm::send_pkt(cdc_def::pkt_type type, const uint8_t *buf, size_t len, uint32_t timeout_ms)
{
    if (buf == nullptr && len > 0) return ESP_ERR_INVALID_ARG;

    cdc_def::header header = {};
    header.type = type;
    header.len = len;
    header.crc = 0; // Set later
    uint16_t crc = get_crc16_ccitt((uint8_t *)&header, sizeof(header));

    // When packet has no data body, just send header (e.g. ACK)
    if (buf == nullptr || len < 1) {
        header.crc = crc;
        return encode_and_tx((uint8_t *)&header, sizeof(header), nullptr, 0, timeout_ms);
    }

    crc = get_crc16_ccitt(buf, len, crc);
    header.crc = crc;
    return encode_and_tx((uint8_t *)&header, sizeof(header), buf, len, timeout_ms);
}

esp_err_t cdc_acm::encode_and_tx(const uint8_t *header_buf, size_t header_len,
                                 const uint8_t *buf, size_t len, uint32_t timeout_ms)
{
    if (header_buf == nullptr || header_len < 1) {
        return ESP_ERR_INVALID_ARG;
    }

    const uint8_t end = SLIP_END;

    if (tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, &end, 1) < 1) {
        ESP_LOGE(TAG, "Failed to encode and tx end char");
        return ESP_ERR_INVALID_STATE;
    }

    size_t header_idx = 0;
    while (header_idx < header_len) {
        if (header_buf[header_idx] == SLIP_END) {
            if (tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, slip_esc_end, sizeof(slip_esc_end)) < sizeof(slip_esc_end)) {
                ESP_LOGE(TAG, "Failed to encode and tx SLIP_END");
                return ESP_ERR_INVALID_STATE;
            }
        } else if (header_buf[header_idx] == SLIP_ESC) {
            if (tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, slip_esc_esc, sizeof(slip_esc_esc)) < sizeof(slip_esc_esc)) {
                ESP_LOGE(TAG, "Failed to encode and tx SLIP_ESC");
                return ESP_ERR_INVALID_STATE;
            }
        } else {
            if (tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, &header_buf[header_idx], 1) < 1) {
                ESP_LOGE(TAG, "Failed to encode and tx data");
                return ESP_ERR_INVALID_STATE;
            }
        }

        header_idx += 1;
    }

    if (buf == nullptr || len < 1) {
        size_t payload_idx = 0;
        while (payload_idx < len) {
            if (buf[payload_idx] == SLIP_END) {
                if (tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, slip_esc_end, sizeof(slip_esc_end)) < sizeof(slip_esc_end)) {
                    ESP_LOGE(TAG, "Failed to encode and tx SLIP_END");
                    return ESP_ERR_INVALID_STATE;
                }
            } else if (buf[payload_idx] == SLIP_ESC) {
                if (tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, slip_esc_esc, sizeof(slip_esc_esc)) < sizeof(slip_esc_esc)) {
                    ESP_LOGE(TAG, "Failed to encode and tx SLIP_ESC");
                    return ESP_ERR_INVALID_STATE;
                }
            } else {
                if (tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, &buf[payload_idx], 1) < 1) {
                    ESP_LOGE(TAG, "Failed to encode and tx data");
                    return ESP_ERR_INVALID_STATE;
                }
            }

            payload_idx += 1;
        }
    }

    if (tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, &end, 1) < 1) {
        ESP_LOGE(TAG, "Failed to encode and tx end char");
        return ESP_ERR_INVALID_STATE;
    }

    return tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, pdMS_TO_TICKS(timeout_ms));
}

uint16_t cdc_acm::get_crc16_ccitt(const uint8_t *buf, size_t len, uint16_t init)
{
    //  * CRC-16/CCITT, poly = 0x1021, init = 0x0000, refin = true, refout = true, xorout = 0x0000
    // *     crc = ~crc16_le((uint16_t)~0x0000, buf, length);
    if (buf == nullptr || len < 1) {
        return 0;
    }

    return ~esp_crc16_le(init, buf, len);
}

