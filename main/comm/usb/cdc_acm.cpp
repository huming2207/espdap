#include <esp_log.h>
#include <esp_crc.h>
#include <esp_flash.h>
#include <esp_mac.h>

#include "cdc_acm.hpp"
#include "file_utils.hpp"

esp_err_t cdc_acm::init(tinyusb_cdcacm_itf_t channel)
{
    cdc_channel = channel;
    static char sn_str[32] = {};
    static char lang[2] = {0x09, 0x04};

    static char *desc_str[5] = {
            lang,                // 0: is supported language is English (0x0409)
            const_cast<char *>(USB_DESC_MANUFACTURER), // 1: Manufacturer
            const_cast<char *>(USB_DESC_PRODUCT),      // 2: Product
            sn_str,       // 3: Serials, should use chip ID
            const_cast<char *>(USB_DESC_CDC_NAME),          // 4: CDC Interface
    };

    tinyusb_config_t tusb_cfg = {}; // the configuration using default values
    tusb_cfg.string_descriptor = (const char **)desc_str;
    tusb_cfg.device_descriptor = nullptr;
    tusb_cfg.self_powered = false;
    tusb_cfg.external_phy = false;

    uint8_t sn_buf[16] = { 0 };
    esp_efuse_mac_get_default(sn_buf);
    esp_flash_read_unique_chip_id(esp_flash_default_chip, reinterpret_cast<uint64_t *>(sn_buf + 6));

    snprintf(sn_str, 32, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
             sn_buf[0], sn_buf[1], sn_buf[2], sn_buf[3], sn_buf[4], sn_buf[5], sn_buf[6], sn_buf[7],
             sn_buf[8], sn_buf[9], sn_buf[10], sn_buf[11], sn_buf[12], sn_buf[13]);

    ESP_LOGI(TAG, "Initialised with SN: %s", sn_str);

    rx_ringbuf_buf = (uint8_t *)heap_caps_calloc(RX_DECODED_RING_BUFFER_SIZE, 1, MALLOC_CAP_SPIRAM);
    if (rx_ringbuf_buf == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate Rx ringbuffer");
        return ESP_ERR_NO_MEM;
    }

    rx_ringbuf = xRingbufferCreateStatic(RX_DECODED_RING_BUFFER_SIZE, RINGBUF_TYPE_BYTEBUF, rx_ringbuf_buf, &rx_ringbuf_ctx);
    if (rx_ringbuf == nullptr) {
        ESP_LOGE(TAG, "Failed to set up Rx ringbuffer");
        free(rx_ringbuf_buf);
        return ESP_ERR_NO_MEM;
    }

    tinyusb_msc_storage_mount()

    auto ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TinyUSB driver install failed");
        return ret;
    }

    tinyusb_config_cdcacm_t acm_cfg = {};
    acm_cfg.usb_dev = TINYUSB_USBDEV_0;
    acm_cfg.cdc_port = cdc_channel;
    acm_cfg.rx_unread_buf_sz = 512;
    acm_cfg.callback_rx = &serial_rx_cb;
    acm_cfg.callback_rx_wanted_char = nullptr;
    acm_cfg.callback_line_state_changed = nullptr;
    acm_cfg.callback_line_coding_changed = nullptr;

    ret = ret ?: tusb_cdc_acm_init(&acm_cfg);

    io_events = xEventGroupCreate();
    if (io_events == nullptr) {
        ESP_LOGE(TAG, "Failed to create Rx event group");
        return ESP_ERR_NO_MEM;
    }

    return ret;
}

esp_err_t cdc_acm::begin_send(size_t len, uint32_t timeout_ticks)
{
    if (xEventGroupWaitBits(io_events, cdc_def::EVT_TX_IDLE, pdTRUE, pdTRUE, timeout_ticks) == 0) {
        return ESP_ERR_TIMEOUT;
    }

    xEventGroupSetBits(io_events, cdc_def::EVT_TX_BEGIN);

    const uint8_t start = SLIP_START;
    if (tinyusb_cdcacm_write_queue(cdc_channel, &start, 1) < 1) {
        ESP_LOGE(TAG, "Failed to encode and tx start char");
        xEventGroupSetBits(io_events, cdc_def::EVT_TX_IDLE);
        xEventGroupClearBits(io_events, cdc_def::EVT_TX_BEGIN);
        return ESP_ERR_NOT_FINISHED;
    }

    return ESP_OK;
}

esp_err_t cdc_acm::send_buf(const uint8_t *buf, size_t len, uint32_t timeout_ticks)
{
    if (xEventGroupWaitBits(io_events, cdc_def::EVT_TX_BEGIN, pdFALSE, pdTRUE, timeout_ticks) == 0) {
        return ESP_ERR_TIMEOUT;
    }

    xEventGroupClearBits(io_events, cdc_def::EVT_TX_IDLE);

    const uint8_t slip_esc_start[] = { SLIP_ESC, SLIP_ESC_START };
    const uint8_t slip_esc_end[] = { SLIP_ESC, SLIP_ESC_END };
    const uint8_t slip_esc_esc[] = { SLIP_ESC, SLIP_ESC_ESC };

    size_t idx = 0;
    while (idx < len) {
        switch (buf[idx]) {
            case SLIP_START: {
                if (tinyusb_cdcacm_write_queue(cdc_channel, slip_esc_start, sizeof(slip_esc_end)) < sizeof(slip_esc_end)) {
                    ESP_LOGE(TAG, "Failed to encode and tx SLIP_START");
                    xEventGroupSetBits(io_events, cdc_def::EVT_TX_IDLE);
                    xEventGroupClearBits(io_events, cdc_def::EVT_TX_BEGIN);
                    return ESP_ERR_NOT_FINISHED;
                }

                break;
            }

            case SLIP_ESC: {
                if (tinyusb_cdcacm_write_queue(cdc_channel, slip_esc_esc, sizeof(slip_esc_esc)) < sizeof(slip_esc_esc)) {
                    ESP_LOGE(TAG, "Failed to encode and tx SLIP_ESC");
                    xEventGroupSetBits(io_events, cdc_def::EVT_TX_IDLE);
                    xEventGroupClearBits(io_events, cdc_def::EVT_TX_BEGIN);
                    return ESP_ERR_NOT_FINISHED;
                }

                break;
            }

            case SLIP_END: {
                if (tinyusb_cdcacm_write_queue(cdc_channel, slip_esc_end, sizeof(slip_esc_end)) < sizeof(slip_esc_end)) {
                    ESP_LOGE(TAG, "Failed to encode and tx SLIP_END");
                    xEventGroupSetBits(io_events, cdc_def::EVT_TX_IDLE);
                    xEventGroupClearBits(io_events, cdc_def::EVT_TX_BEGIN);
                    return ESP_ERR_NOT_FINISHED;
                }

                break;
            }

            default: {
                if (tinyusb_cdcacm_write_queue(cdc_channel, &buf[idx], 1) < 1) {
                    ESP_LOGE(TAG, "Failed to encode and tx data");
                    xEventGroupSetBits(io_events, cdc_def::EVT_TX_IDLE);
                    xEventGroupClearBits(io_events, cdc_def::EVT_TX_BEGIN);
                    return ESP_ERR_NOT_FINISHED;
                }

                break;
            }
        }

        idx += 1;
    }

    return ESP_OK;
}

esp_err_t cdc_acm::finish_send(uint32_t timeout_ticks)
{
    if (xEventGroupWaitBits(io_events, cdc_def::EVT_TX_BEGIN, pdFALSE, pdTRUE, timeout_ticks) == 0) {
        return ESP_ERR_TIMEOUT;
    }

    xEventGroupClearBits(io_events, cdc_def::EVT_TX_IDLE);

    const uint8_t end = SLIP_END;
    if (tinyusb_cdcacm_write_queue(cdc_channel, &end, 1) < 1) {
        ESP_LOGE(TAG, "Failed to encode and tx end char");
        xEventGroupSetBits(io_events, cdc_def::EVT_TX_IDLE);
        xEventGroupClearBits(io_events, cdc_def::EVT_TX_BEGIN);
        return ESP_ERR_NOT_FINISHED;
    }

    xEventGroupSetBits(io_events, cdc_def::EVT_TX_IDLE);
    return ESP_OK;
}

esp_err_t cdc_acm::flush_send_queue(uint32_t timeout_ticks)
{
    return tinyusb_cdcacm_write_flush(cdc_channel, timeout_ticks);
}

void cdc_acm::serial_rx_cb(int itf, cdcacm_event_t *event)
{
    auto *ctx = cdc_acm::instance();
    if (ctx->cdc_channel != itf) {
        return;
    }

    size_t rx_size = 0;
    auto ret = tinyusb_cdcacm_read(static_cast<tinyusb_cdcacm_itf_t>(itf), rx_raw_buf, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TinyUSB read fail: 0x%x", ret);
        memset(rx_raw_buf, 0, sizeof(rx_raw_buf));
        return;
    }

    if (rx_size < 1) {
        memset(rx_raw_buf, 0, sizeof(rx_raw_buf));
        return;
    }

    // Start de-SLIP
    size_t idx = 0;
    while (idx < std::min(rx_size, (size_t)CONFIG_TINYUSB_CDC_RX_BUFSIZE)) {
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed when de-SLIP: 0x%x", ret);
            return;
        }

        switch (rx_raw_buf[idx]) {
            case SLIP_START: {
                xEventGroupClearBits(ctx->io_events, cdc_def::EVT_NEW_PACKET);
                xEventGroupSetBits(ctx->io_events, cdc_def::EVT_READING_SLIP_FRAME);

                idx += 1;
                break;
            }

            case SLIP_ESC: {
                // During receive: if it's not started, just ignore it
                if ((xEventGroupGetBits(ctx->io_events) & cdc_def::EVT_READING_SLIP_FRAME) == 0) {
                    break;
                }

                // Skip the ESC byte
                idx += 1;

                // Handle the second bytes
                switch (rx_raw_buf[idx]) {
                    case SLIP_ESC_START: {
                        ret = ctx->write_to_rx_buf(SLIP_START);
                        break;
                    }

                    case SLIP_ESC_END: {
                        ret = ctx->write_to_rx_buf(SLIP_END);
                        break;
                    }

                    case SLIP_ESC_ESC: {
                        ret = ctx->write_to_rx_buf(SLIP_ESC);
                        break;
                    }

                    default: {
                        xEventGroupClearBits(ctx->io_events, cdc_def::EVT_READING_SLIP_FRAME);
                        xEventGroupClearBits(ctx->io_events, cdc_def::EVT_NEW_PACKET);
                        ESP_LOGE(TAG, "Unexpected SLIP ESC: 0x%02x", rx_raw_buf[idx]);
                        return;
                    }
                }

                idx += 1;
                break;
            }

            case SLIP_END: {
                // During receive: if it's not started, just ignore it
                if ((xEventGroupGetBits(ctx->io_events) & cdc_def::EVT_READING_SLIP_FRAME) == 0) {
                    break;
                }

                xEventGroupSetBits(ctx->io_events, cdc_def::EVT_NEW_PACKET);
                xEventGroupClearBits(ctx->io_events, cdc_def::EVT_READING_SLIP_FRAME);

                break;
            }

            default: {
                // During receive: if it's not started, just ignore it
                if ((xEventGroupGetBits(ctx->io_events) & cdc_def::EVT_READING_SLIP_FRAME) == 0) {
                    break;
                }

                ret = ctx->write_to_rx_buf(rx_raw_buf[idx]);
                break;
            }
        }
    }

    memset(rx_raw_buf, 0, sizeof(rx_raw_buf));
}

esp_err_t cdc_acm::pause_recv()
{
    if (!tusb_inited() || paused) return ESP_ERR_INVALID_STATE;
    xEventGroupClearBits(io_events, cdc_def::EVT_NEW_PACKET);

    paused = true;
    return tinyusb_cdcacm_unregister_callback(cdc_channel, CDC_EVENT_RX);
}

esp_err_t cdc_acm::resume_recv()
{
    if (!tusb_inited() || !paused) return ESP_ERR_INVALID_STATE;

    paused = false;
    return tinyusb_cdcacm_register_callback(cdc_channel, CDC_EVENT_RX, serial_rx_cb);
}

esp_err_t cdc_acm::write_to_rx_buf(uint8_t data)
{
    if (xRingbufferSend(rx_ringbuf, &data, 1, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t cdc_acm::wait_for_recv(uint32_t timeout_ticks)
{
    auto ret = xEventGroupWaitBits(io_events, cdc_def::EVT_NEW_PACKET, pdTRUE, pdFALSE, timeout_ticks);
    if ((ret & cdc_def::EVT_NEW_PACKET) == 0) {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t cdc_acm::acquire_read_buf(uint8_t **out, size_t req_len, size_t *actual_len, uint32_t timeout_ticks)
{
    if (out == nullptr || actual_len == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    auto *buf_ptr = (uint8_t *)xRingbufferReceiveUpTo(rx_ringbuf, actual_len, timeout_ticks, req_len);

    if (buf_ptr == nullptr) {
        ESP_LOGE(TAG, "No Rx stuff retrieved");
        return ESP_ERR_TIMEOUT;
    } else {
        *out = buf_ptr;
    }

    return ESP_OK;
}

esp_err_t cdc_acm::release_read_buf(uint8_t *buf, size_t buf_read)
{
    vRingbufferReturnItem(rx_ringbuf, buf);
    return ESP_OK;
}

esp_err_t cdc_acm::clear_recv_buf()
{
    size_t read_len = 0;
    void *buf_ptr = xRingbufferReceiveUpTo(rx_ringbuf, &read_len, 10, RX_DECODED_RING_BUFFER_SIZE);
    if (buf_ptr == nullptr) {
        ESP_LOGW(TAG, "Rx ring buffer is empty, no need to clear?");
        return ESP_ERR_NOT_FOUND; // No-op??
    }

    vRingbufferReturnItem(rx_ringbuf, buf_ptr);
    return ESP_OK;
}
