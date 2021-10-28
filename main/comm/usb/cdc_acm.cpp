#include <esp_log.h>

#include "cdc_acm.hpp"

esp_err_t cdc_acm::init()
{
    tinyusb_config_t tusb_cfg = {}; // the configuration using default values
    auto ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "TinyUSB driver install failed");
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

    return ret;
}

void cdc_acm::serial_rx_cb(int itf, cdcacm_event_t *event)
{
    auto &ctx = cdc_acm::instance();
}

