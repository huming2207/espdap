#pragma once

#include <cstdio>
#include <esp_err.h>
#include <esp_http_client.h>

class http_downloader
{
public:
    enum evt_bits : uint32_t {
        REQ_ERROR = BIT(0),
        REQ_DATA_AVAIL = BIT(1),
        REQ_DONE = BIT(2),
    };

public:
    http_downloader() = default;
    esp_err_t init(const char *_url, const char *_save_path, size_t _max_len = 1048576);
    esp_err_t set_url(const char *url);
    esp_err_t set_method(esp_http_client_method_t method);
    esp_err_t set_header(const char *key, const char *val);
    esp_err_t set_post_field(uint8_t *buf, size_t len);
    esp_err_t request(uint32_t timeout_ticks = pdMS_TO_TICKS(600000));
    ~http_downloader();

private:
    EventGroupHandle_t evt_group = nullptr;
    esp_http_client_handle_t client_ctx = nullptr;
    size_t curr_pos = 0;
    size_t max_len = 0;
    FILE *fp = nullptr;
    static esp_err_t http_evt_handler(esp_http_client_event_t *evt);

    static const constexpr char TAG[] = "http_dl";
};
