#include <cstring>
#include <esp_log.h>

#include <mbedtls/base64.h>
#include "flash_algo.hpp"

using namespace ArduinoJson;

esp_err_t flash_algo::init(const char *path)
{
    FILE *file = fopen(path, "r");
    if (file == nullptr) {
        ESP_LOGE(TAG, "Failed when reading file");
        return ESP_ERR_NOT_FOUND;
    }

    fseek(file, 0, SEEK_END);
    size_t len = ftell(file);
    if (len < 1 || len > FLASH_ALGO_MAX_FILE_SIZE - 1) {
        ESP_LOGE(TAG, "Flash algorithm is in a wrong length: %u", len);
        return ESP_ERR_INVALID_STATE;
    }

    fseek(file, 0, SEEK_SET);
    fread(algo_buf, 1, len, file);
    fclose(file);

    if (deserializeJson(json_doc, algo_buf, FLASH_ALGO_MAX_FILE_SIZE) != DeserializationError::Ok) {
        ESP_LOGE(TAG, "Failed to parse flash algorithm JSON");
        return ESP_ERR_INVALID_STATE;
    }

    const char *algo_encoded = json_doc["instructions"].as<const char *>();
    if (algo_encoded == nullptr) {
        ESP_LOGE(TAG, "Flash algo not found or corrupted!");
        return ESP_ERR_NOT_FOUND;
    }

    size_t algo_encoded_len = strlen(algo_encoded);
    if (algo_encoded_len < 1) {
        ESP_LOGE(TAG, "Flash algo too short");
        return ESP_ERR_INVALID_SIZE;
    }

    if (mbedtls_base64_decode(
            algo_bin, algo_encoded_len, &algo_bin_len,
            (uint8_t *)algo_encoded, algo_encoded_len) < 0) {
        ESP_LOGE(TAG, "Flash algorithm base64 decode fail");
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

flash_algo::~flash_algo()
{
    delete[] algo_buf;
}

const char *flash_algo::get_algo_name() const
{
    return json_doc["name"].as<const char *>();
}

const char *flash_algo::get_target_name() const
{
    return json_doc["description"].as<const char *>();
}

uint8_t *flash_algo::get_algo_bin() const
{
    return algo_bin;
}

uint32_t flash_algo::get_algo_bin_len() const
{
    return algo_bin_len;
}

uint32_t flash_algo::get_ram_size_byte() const
{
    return json_doc["ramSize"].as<uint32_t>();
}

uint32_t flash_algo::get_flash_size_byte() const
{
    return json_doc["flashSize"].as<uint32_t>();
}

uint32_t flash_algo::get_pc_init() const
{
    return json_doc["pcInit"].as<uint32_t>();
}

uint32_t flash_algo::get_pc_uninit() const
{
    return json_doc["pcUninit"].as<uint32_t>();
}

uint32_t flash_algo::get_pc_program_page() const
{
    return json_doc["pcProgramPage"].as<uint32_t>();
}

uint32_t flash_algo::get_pc_erase_sector() const
{
    return json_doc["pcEraseSector"].as<uint32_t>();
}

uint32_t flash_algo::get_pc_erase_all() const
{
    return json_doc["pcEraseAll"].as<uint32_t>();
}

uint32_t flash_algo::get_data_section_offset() const
{
    return json_doc["dataSectionOffset"].as<uint32_t>();
}

uint32_t flash_algo::get_flash_start_addr() const
{
    return json_doc["flashStartAddr"].as<uint32_t>();
}

uint32_t flash_algo::get_flash_end_addr() const
{
    return json_doc["flashEndAddr"].as<uint32_t>();
}

uint32_t flash_algo::get_page_size() const
{
    return json_doc["flashPageSize"].as<uint32_t>();
}

uint32_t flash_algo::get_erased_byte_val() const
{
    return json_doc["erasedByteValue"].as<uint32_t>();
}

uint32_t flash_algo::get_program_page_timeout() const
{
    return json_doc["programTimeout"].as<uint32_t>();
}

uint32_t flash_algo::get_erase_sector_timeout() const
{
    return json_doc["eraseTimeout"].as<uint32_t>();
}

uint32_t flash_algo::get_sector_size() const
{
    return json_doc["flashSectorSize"].as<uint32_t>();
}
