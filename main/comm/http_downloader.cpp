#include <cstdio>
#include <esp_log.h>
#include "http_downloader.hpp"

esp_err_t http_downloader::init(const char *_url, const char *_save_path, size_t _max_len)
{
    if (_save_path == nullptr || _url == nullptr || _max_len < 1) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_http_client_config_t config = {};
    config.url = _url;
    config.disable_auto_redirect = false;
    config.event_handler = http_evt_handler;
    config.user_data = this;
    config.user_agent = "SoulInjector/5.0";
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 600000; // In case I'm in China...
    config.buffer_size = DEFAULT_HTTP_BUF_SIZE;
    config.buffer_size_tx = DEFAULT_HTTP_BUF_SIZE;

    evt_group = xEventGroupCreate();
    if (evt_group == nullptr) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_ERR_NO_MEM;
    }

    client_ctx = esp_http_client_init(&config);
    if (client_ctx == nullptr) {
        ESP_LOGE(TAG, "Failed to set up ESP http client");
        return ESP_ERR_NO_MEM;
    }

    max_len = _max_len;
    fp = fopen(_save_path, "w+");
    if (fp == nullptr) {
        ESP_LOGE(TAG, "Failed to save file");
        return ESP_ERR_NO_MEM;
    }


    return ESP_OK;
}

esp_err_t http_downloader::set_url(const char *_url)
{
    return esp_http_client_set_url(client_ctx, _url);
}

esp_err_t http_downloader::set_method(esp_http_client_method_t method)
{
    return esp_http_client_set_method(client_ctx, method);
}

esp_err_t http_downloader::set_header(const char *key, const char *val)
{
    return esp_http_client_set_header(client_ctx, key, val);
}

esp_err_t http_downloader::set_post_field(uint8_t *buf, size_t len)
{
    return esp_http_client_set_post_field(client_ctx, (const char *)buf, (int)len);
}

esp_err_t http_downloader::request(uint32_t timeout_ticks)
{
    ESP_LOGI(TAG, "Start request fetching stuff!");
    auto ret = esp_http_client_perform(client_ctx);
    ESP_LOGW(TAG, "End request, ret=0x%x %s", ret, esp_err_to_name(ret));
    if (ret != ESP_OK) {
        return ret;
    }

    EventBits_t bits = xEventGroupWaitBits(evt_group, (http_downloader::REQ_DONE | http_downloader::REQ_ERROR), pdTRUE, pdFALSE, timeout_ticks);
    if ((bits & http_downloader::REQ_DONE) == 0) {
        ESP_LOGE(TAG, "Failed to download - timeout!");
        return ESP_ERR_TIMEOUT;
    }

    return ret;
}

esp_err_t http_downloader::http_evt_handler(esp_http_client_event_t *evt)
{
    if (evt == nullptr || evt->user_data == nullptr) {
        ESP_LOGE(TAG, "Invalid context ptr");
        return ESP_ERR_INVALID_STATE;
    }

    auto *ctx = (http_downloader *)evt->user_data;
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR: {
            xEventGroupClearBits(ctx->evt_group, (http_downloader::REQ_DATA_AVAIL | http_downloader::REQ_DONE));
            xEventGroupSetBits(ctx->evt_group, http_downloader::REQ_ERROR);
            break;
        }

        case HTTP_EVENT_ON_DATA: {
            if ((ctx->curr_pos + evt->data_len) > ctx->max_len) {
                ESP_LOGE(TAG, "Exceeding max length! max=%u, now=%u", ctx->max_len, (ctx->curr_pos + evt->data_len));
                xEventGroupClearBits(ctx->evt_group, (http_downloader::REQ_DATA_AVAIL | http_downloader::REQ_DONE));
                xEventGroupSetBits(ctx->evt_group, http_downloader::REQ_ERROR);
                return ESP_ERR_NO_MEM;
            }

            if (ctx->fp == nullptr) {
                ESP_LOGE(TAG, "Invalid file pointer");
                xEventGroupClearBits(ctx->evt_group, (http_downloader::REQ_DATA_AVAIL | http_downloader::REQ_DONE));
                xEventGroupSetBits(ctx->evt_group, http_downloader::REQ_ERROR);
                return ESP_ERR_INVALID_STATE;
            }

            size_t ret = fwrite(evt->data, evt->data_len, 1, ctx->fp);
            if (ret < 1) {
                ESP_LOGE(TAG, "Something wrong when saving file, ret=%d", ret);
                xEventGroupClearBits(ctx->evt_group, (http_downloader::REQ_DATA_AVAIL | http_downloader::REQ_DONE));
                xEventGroupSetBits(ctx->evt_group, http_downloader::REQ_ERROR);
                return ESP_ERR_INVALID_STATE;
            } else {
                ctx->curr_pos += evt->data_len;
            }

            ESP_LOGI(TAG, "Written %u, pos=%d", evt->data_len, ctx->curr_pos);
            xEventGroupClearBits(ctx->evt_group, http_downloader::REQ_ERROR);
            xEventGroupSetBits(ctx->evt_group, http_downloader::REQ_DATA_AVAIL);
            break;
        }

        case HTTP_EVENT_ON_FINISH: {
            ESP_LOGI(TAG, "Request finished, flushing fp");
            if (ctx->fp == nullptr) {
                return ESP_OK; // Make it no-op
            }

            fflush(ctx->fp);
            fclose(ctx->fp);
            ctx->fp = nullptr;
            xEventGroupClearBits(ctx->evt_group, http_downloader::REQ_ERROR);
            xEventGroupSetBits(ctx->evt_group, http_downloader::REQ_DONE);
            ESP_LOGI(TAG, "Request finished, fp closed");
            break;
        }

        case HTTP_EVENT_DISCONNECTED: {
            ESP_LOGI(TAG, "HTTP Disconnected");
            if (ctx->fp == nullptr) {
                return ESP_OK; // Make it no-op
            }

            fflush(ctx->fp);
            fclose(ctx->fp);
            ctx->fp = nullptr;
            break;
        }

        default: {
            break;
        }
    }

    return ESP_OK;

}

http_downloader::~http_downloader()
{
    if (fp != nullptr) {
        fflush(fp);
        fclose(fp);
    }

    if (client_ctx != nullptr) {
        esp_http_client_close(client_ctx);
        esp_http_client_cleanup(client_ctx);
    }

    if (evt_group != nullptr) {
        free(evt_group);
    }
}
