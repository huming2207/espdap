#include <esp_log.h>
#include <esp_crc.h>
#include <esp_flash.h>

#include "cdc_acm.hpp"
#include "config_manager.hpp"


esp_err_t cdc_acm::init()
{
    static char sn_str[32] = {};
    static const char lang[2] = {0x09, 0x04};
    static tusb_desc_strarray_device_t desc_str = {
            // array of pointer to string descriptors
            lang,                // 0: is supported language is English (0x0409)
            "Jackson Hu", // 1: Manufacturer
            "Soul Injector Programmer",      // 2: Product
            sn_str,       // 3: Serials, should use chip ID
            "Soul Injector Programmer",          // 4: CDC Interface
            "",
            "",
    };
    static tusb_desc_device_t desc_device = {};

    tinyusb_config_t tusb_cfg = {}; // the configuration using default values
    tusb_cfg.string_descriptor = desc_str;

    uint8_t sn_buf[16] = { 0 };
    esp_efuse_mac_get_default(sn_buf);
    esp_flash_read_unique_chip_id(esp_flash_default_chip, reinterpret_cast<uint64_t *>(sn_buf + 6));

    snprintf(sn_str, 32, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
             sn_buf[0], sn_buf[1], sn_buf[2], sn_buf[3], sn_buf[4], sn_buf[5], sn_buf[6], sn_buf[7],
             sn_buf[8], sn_buf[9], sn_buf[10], sn_buf[11], sn_buf[12], sn_buf[13]);

    ESP_LOGI(TAG, "Initialised with SN: %s", sn_str);

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

    xTaskCreate(rx_handler_task, "cdc_rx", 8192, this, tskIDLE_PRIORITY + 1, nullptr);

    decoded_buf = static_cast<uint8_t *>(heap_caps_malloc(CONFIG_TINYUSB_CDC_RX_BUFSIZE, MALLOC_CAP_SPIRAM));
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

    ESP_LOGI(TAG, "Before SLIP:");
    ESP_LOG_BUFFER_HEX(TAG, rx_buf, rx_size);

    if (rx_size < 1) return;

    size_t idx = 0;
    while (idx < rx_size && ctx.curr_rx_len < CONFIG_TINYUSB_CDC_RX_BUFSIZE) {
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

            ctx.curr_rx_len += 1;
        } else {
            ctx.decoded_buf[ctx.curr_rx_len] = rx_buf[idx];
            ctx.curr_rx_len += 1;
        }

        idx += 1;
    }
}

[[noreturn]] void cdc_acm::rx_handler_task(void *_ctx)
{
    ESP_LOGI(TAG, "Rx handler task started");
    auto &ctx = cdc_acm::instance();
    while(true) {
        if (xEventGroupWaitBits(ctx.rx_event, cdc_def::EVT_NEW_PACKET, pdTRUE, pdFALSE, portMAX_DELAY) == pdTRUE) {
            // Pause Rx
            tinyusb_cdcacm_unregister_callback(TINYUSB_CDC_ACM_0, CDC_EVENT_RX);

            ESP_LOGI(TAG, "Now in buffer:");
            ESP_LOG_BUFFER_HEX(TAG, ctx.decoded_buf, ctx.curr_rx_len);

            // Now do parsing
            ctx.parse_pkt();

            // Clear up the mess
            ctx.curr_rx_len = 0;
            memset(ctx.decoded_buf, 0, CONFIG_TINYUSB_CDC_RX_BUFSIZE);

            // Restart Rx
            tinyusb_cdcacm_register_callback(TINYUSB_CDC_ACM_0, CDC_EVENT_RX, serial_rx_cb);
        }
    }
}

esp_err_t cdc_acm::send_ack(uint16_t crc, uint32_t timeout_ms)
{
    return send_pkt(cdc_def::PKT_ACK, nullptr, 0);
}

esp_err_t cdc_acm::send_nack(uint32_t timeout_ms)
{
    return send_pkt(cdc_def::PKT_NACK, nullptr, 0);
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

esp_err_t cdc_acm::send_chunk_ack(cdc_def::chunk_ack state, uint32_t aux, uint32_t timeout_ms)
{
    cdc_def::chunk_ack_pkt pkt = {};
    pkt.aux_info = aux;
    pkt.state = state;

    return send_pkt(cdc_def::PKT_CHUNK_ACK, (uint8_t *)&pkt, sizeof(pkt), timeout_ms);
}

esp_err_t cdc_acm::send_pkt(cdc_def::pkt_type type, const uint8_t *buf, size_t len, uint32_t timeout_ms)
{
    if (buf == nullptr && len > 0) return ESP_ERR_INVALID_ARG;

    cdc_def::header header = {};
    header.type = type;
    header.len = len;
    header.crc = 0; // Set later
    uint16_t crc = get_crc16((uint8_t *) &header, sizeof(header));

    // When packet has no data body, just send header (e.g. ACK)
    if (buf == nullptr || len < 1) {
        header.crc = crc;
        return encode_and_tx((uint8_t *)&header, sizeof(header), nullptr, 0, timeout_ms);
    } else {
        crc = get_crc16(buf, len, crc);
        header.crc = crc;
        return encode_and_tx((uint8_t *)&header, sizeof(header), buf, len, timeout_ms);
    }
}

esp_err_t cdc_acm::encode_and_tx(const uint8_t *header_buf, size_t header_len,
                                 const uint8_t *buf, size_t len, uint32_t timeout_ms)
{
    const uint8_t slip_esc_end[] = { SLIP_ESC, SLIP_ESC_END };
    const uint8_t slip_esc_esc[] = { SLIP_ESC, SLIP_ESC_ESC };

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

    if (buf != nullptr && len > 1) {
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

uint16_t cdc_acm::get_crc16(const uint8_t *buf, size_t len, uint16_t init)
{
//  * CRC-16/XMODEM, poly= 0x1021, init = 0x0000, refin = false, refout = false, xorout = 0x0000
// *     crc = ~crc16_be((uint16_t)~0x0000, buf, length);
    if (buf == nullptr || len < 1) {
        return 0;
    }

    return ~esp_crc16_be((uint16_t)~init, buf, len);
}

void cdc_acm::parse_pkt()
{
    if (curr_rx_len < sizeof(cdc_def::header)) {
        ESP_LOGW(TAG, "Packet too short, failed to decode header: %u", curr_rx_len);
        recv_state = cdc_def::FILE_RECV_NONE;
        send_nack();
        return;
    }

    auto *header = (cdc_def::header *)decoded_buf;

    uint16_t expected_crc = header->crc;
    header->crc = 0;

    uint16_t actual_crc = get_crc16(decoded_buf, curr_rx_len);
    if (actual_crc != expected_crc) {
        ESP_LOGW(TAG, "Incoming packet CRC corrupted, expect 0x%x, actual 0x%x", expected_crc, actual_crc);
        send_nack();
        return;
    }

    if (recv_state != cdc_def::FILE_RECV_NONE && header->type != cdc_def::PKT_DATA_CHUNK) {
        ESP_LOGW(TAG, "Invalid state - data chunk expected while received type 0x%x", header->type);
        send_nack();
        return;
    }

    switch (header->type) {
        case cdc_def::PKT_PING: {
            send_ack();
            break;
        }

        case cdc_def::PKT_DEVICE_INFO: {
            send_dev_info();
            break;
        }

        case cdc_def::PKT_GET_CONFIG: {
            parse_get_config();
            break;
        }

        case cdc_def::PKT_SET_CONFIG: {
            parse_set_config();
            break;
        }

        case cdc_def::PKT_SET_ALGO_METADATA: {
            parse_set_algo_metadata();
            break;
        }

        case cdc_def::PKT_GET_ALGO_METADATA: {
            parse_get_algo_info();
            break;
        }

        case cdc_def::PKT_SET_FW_METADATA: {
            parse_set_fw_metadata();
            break;
        }

        case cdc_def::PKT_GET_FW_METADATA:{
            parse_get_fw_info();
            break;
        }

        case cdc_def::PKT_DATA_CHUNK: {
            if (recv_state != cdc_def::FILE_RECV_NONE) {
                parse_chunk();
            } else {
                ESP_LOGW(TAG, "Invalid state - no chunk expected to come or should have EOL'ed??");
                send_chunk_ack(cdc_def::CHUNK_ERR_UNEXPECTED, 0);
            }
            break;
        }

        default: {
            ESP_LOGW(TAG, "Unknown packet type 0x%x received", header->type);
            send_nack();
            break;
        }
    }
}

void cdc_acm::parse_get_config()
{

}

void cdc_acm::parse_set_config()
{

}

void cdc_acm::parse_get_algo_info()
{

}

void cdc_acm::parse_set_algo_metadata()
{
    auto *algo_info = (cdc_def::algo_info *)(decoded_buf + sizeof(cdc_def::header));
    if (algo_info->len > CFG_MGR_FLASH_ALGO_MAX_SIZE) {
        ESP_LOGE(TAG, "Flash algo metadata len too long: %u", algo_info->len);
        send_nack();
        return;
    }

    chunk_expect_len = algo_info->len;
    chunk_crc = algo_info->crc;
    chunk_buf = static_cast<uint8_t *>(malloc(algo_info->len));
    memset(chunk_buf, 0, algo_info->len);
    recv_state = cdc_def::FILE_RECV_ALGO;
    send_chunk_ack(cdc_def::CHUNK_XFER_NEXT, 0);
}

void cdc_acm::parse_get_fw_info()
{

}

void cdc_acm::parse_set_fw_metadata()
{
    auto *fw_info = (cdc_def::fw_info *)(decoded_buf + sizeof(cdc_def::header));
    if (fw_info->len > CFG_MGR_FLASH_ALGO_MAX_SIZE) {
        ESP_LOGE(TAG, "Firmware metadata len too long: %u", fw_info->len);
        send_nack();
        return;
    }

    chunk_expect_len = fw_info->len;
    chunk_crc = fw_info->crc;
    chunk_buf = static_cast<uint8_t *>(malloc(fw_info->len));
    memset(chunk_buf, 0, fw_info->len);
    recv_state = cdc_def::FILE_RECV_FW;
    send_chunk_ack(cdc_def::CHUNK_XFER_NEXT, 0);
}

void cdc_acm::parse_chunk()
{
    auto *chunk = (cdc_def::chunk_pkt *)(decoded_buf + sizeof(cdc_def::header));

    // Scenario 0: if len == 0 then that's force abort, discard the buffer and set back the states
    if (unlikely(chunk->len == 0)) {
        ESP_LOGE(TAG, "Zero len chunk - force abort!");
        chunk_expect_len = 0;
        chunk_curr_offset = 0;
        chunk_crc = 0;
        if (chunk_buf != nullptr) {
            free(chunk_buf);
            chunk_buf = nullptr;
        }
        recv_state = cdc_def::FILE_RECV_NONE;
        send_chunk_ack(cdc_def::CHUNK_ERR_UNEXPECTED, 0);
        return;
    }

    // Scenario 1: if len is too long, reject & abort.
    if (unlikely(chunk->len + chunk_curr_offset > chunk_expect_len)) {
        ESP_LOGE(TAG, "Chunk recv buffer is full, incoming %u while expect %u only", chunk->len + chunk_curr_offset, chunk_expect_len);
        chunk_expect_len = 0;
        chunk_curr_offset = 0;
        chunk_crc = 0;
        if (chunk_buf != nullptr) {
            free(chunk_buf);
            chunk_buf = nullptr;
        }
        recv_state = cdc_def::FILE_RECV_NONE;
        send_chunk_ack(cdc_def::CHUNK_ERR_UNEXPECTED, chunk->len + chunk_curr_offset);
        return;
    }

    // Scenario 2: Normal recv
    memcpy(chunk_buf + chunk_curr_offset, chunk->buf, chunk_expect_len);
    chunk_curr_offset += chunk->len; // Add offset
    if (chunk_curr_offset == chunk_expect_len) {
        // Check full file CRC here
        auto actual_crc = esp_crc32_le(0, chunk_buf, chunk_curr_offset);
        if (actual_crc == chunk_crc) {
            ESP_LOGI(TAG, "Chunk recv successful, got %u bytes", chunk_curr_offset);

            auto ret = ESP_OK;
            if (recv_state == cdc_def::FILE_RECV_ALGO) {
                auto &cfg_mgr = config_manager::instance();
                ret = cfg_mgr.set_algo_bin(chunk_buf, chunk_expect_len);
                ret = ret ?: cfg_mgr.set_algo_bin_len(chunk_expect_len);
            } else if(recv_state == cdc_def::FILE_RECV_FW) {
                // TODO: handle finished firmware file here
            }


            free(chunk_buf);
            chunk_buf = nullptr;
            chunk_expect_len = 0;
            chunk_curr_offset = 0;
            chunk_crc = 0;
            recv_state = cdc_def::FILE_RECV_NONE;

            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Error occur when processing recv buffer: 0x%x", ret);
                send_chunk_ack(cdc_def::CHUNK_ERR_UNEXPECTED, 0);
            } else {
                ESP_LOGI(TAG, "Chunk transfer done!");
                send_chunk_ack(cdc_def::CHUNK_XFER_DONE, chunk_curr_offset);
            }
        } else {
            ESP_LOGE(TAG, "Chunk recv CRC mismatched!!");
            send_chunk_ack(cdc_def::CHUNK_ERR_CRC32_FAIL, actual_crc);
        }
    } else {
        ESP_LOGI(TAG, "Chunk recv - await next @ %u", chunk_curr_offset);
        send_chunk_ack(cdc_def::CHUNK_XFER_NEXT, chunk_curr_offset);
    }
}




