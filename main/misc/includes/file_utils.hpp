#pragma once

#include <esp_err.h>

class file_utils
{
public:
    static esp_err_t validate_firmware_file(const char *path, uint32_t crc)
    {
        if (path == nullptr) {
            ESP_LOGE(TAG, "Path is null!");
            return ESP_ERR_INVALID_ARG;
        }

        FILE *file = fopen(path, "rb");
        if (file == nullptr) {
            ESP_LOGE(TAG, "Failed when reading firmware file");
            return ESP_ERR_NOT_FOUND;
        }

        fseek(file, 0, SEEK_END);
        size_t len = ftell(file);
        if (len < 4 || len % 4 != 0) {
            ESP_LOGE(TAG, "Manifest in a wrong length: %u", len);
            fclose(file);
            return ESP_ERR_INVALID_STATE;
        }

        fseek(file, 0, SEEK_SET);


        uint32_t actual_crc = 0;
        while(len > 0) {
            uint8_t buf[1024] = { 0 };
            auto read_len = std::min(len, (uint32_t)sizeof(buf));
            auto actual_read = fread(buf, 1, read_len, file);
            actual_crc = esp_crc32_le(actual_crc, buf, actual_read);
            len -= actual_read;
        }

        if (actual_crc != crc) {
            ESP_LOGE(TAG, "CRC mismatch for firmware %s, expected 0x%x, actual 0x%x", path, crc, actual_crc);
            return ESP_ERR_INVALID_CRC;
        } else {
            ESP_LOGI(TAG, "CRC matched for firmware %s, expected 0x%x, actual 0x%x", path, crc, actual_crc);
        }

        return ESP_OK;
    }
};