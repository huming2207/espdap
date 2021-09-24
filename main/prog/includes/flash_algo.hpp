#pragma once

#include <cstdint>
#include <cstddef>
#include <esp_err.h>
#include <ArduinoJson.hpp>

#define FLASH_ALGO_MAX_FILE_SIZE 8192
#define FLASH_ALGO_JSON_DOC_SIZE 10240
#define FLASH_ALGO_BIN_SIZE 6150

struct sector_info
{
    uint32_t size = 0;
    uint32_t addr = 0;
};

class flash_algo
{
public:
    flash_algo() = default;
    ~flash_algo();
    esp_err_t init(const char *path);
    const char *get_algo_name() const;
    const char *get_target_name() const;
    uint8_t *get_algo_bin() const;
    uint32_t get_algo_bin_len() const;
    uint32_t get_ram_size_byte() const;
    uint32_t get_flash_size_byte() const;
    uint32_t get_pc_init() const;
    uint32_t get_pc_uninit() const;
    uint32_t get_pc_program_page() const;
    uint32_t get_pc_erase_sector() const;
    uint32_t get_pc_erase_all() const;
    uint32_t get_data_section_offset() const;
    uint32_t get_flash_start_addr() const;
    uint32_t get_flash_end_addr() const;
    uint32_t get_page_size() const;
    uint32_t get_erased_byte_val() const;
    uint32_t get_program_page_timeout() const;
    uint32_t get_erase_sector_timeout() const;
    uint32_t get_sector_size() const;

private:
    static const constexpr char *TAG = "flash_algo";
    ArduinoJson::DynamicJsonDocument json_doc = ArduinoJson::DynamicJsonDocument(FLASH_ALGO_JSON_DOC_SIZE);
    char *algo_buf = new char[FLASH_ALGO_MAX_FILE_SIZE];
    uint8_t *algo_bin = new uint8_t[FLASH_ALGO_BIN_SIZE];
    uint32_t algo_bin_len = 0;
};
