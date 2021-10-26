#pragma once

#include <nvs_handle.hpp>
#include <nvs_flash.h>

class soul_nvs
{
public:
    static soul_nvs& instance()
    {
        static soul_nvs instance;
        return instance;
    }

    soul_nvs(soul_nvs const &) = delete;
    void operator=(soul_nvs const &) = delete;

private:
    soul_nvs() = default;
    std::shared_ptr<nvs::NVSHandle> _nvs;
    bool has_inited = false;

public:
    esp_err_t init()
    {
        if (has_inited) return ESP_OK;

        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            // NVS partition was truncated and needs to be erased
            // Retry nvs_flash_init
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }

        if (ret != ESP_OK) {
            return ret;
        }

        _nvs = nvs::open_nvs_handle("soul", NVS_READWRITE, &ret);
        if (ret == ESP_OK) has_inited = true;

        return ret;
    }

    std::shared_ptr<nvs::NVSHandle> nvs() const
    {
        return _nvs;
    }
};
