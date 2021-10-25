#pragma once

#include <cstdint>
#include <cstddef>
#include <esp_err.h>
#include <soul_nvs.hpp>
#include <driver/gpio.h>

#define FW_MGR_MAGIC_NUMBER 0x4a485349

namespace fw_def
{
    struct __attribute__((packed)) flash_algo_cfg
    {
        uint32_t magic;
        uint32_t pc_init;
        uint32_t pc_uninit;
        uint32_t pc_program_page;
        uint32_t pc_erase_sector;
        uint32_t pc_erase_all;
        uint32_t data_section_offset;
        uint32_t flash_start_addr;
        uint32_t flash_end_addr;
        uint32_t flash_page_size;
        uint32_t erased_byte;
        uint32_t flash_sector_size;
        uint32_t program_timeout;
        uint32_t erase_timeout;
        uint32_t ram_size;
        uint32_t flash_size;
        char name[32];
        char description[32];
    };

    struct __attribute__((packed)) algo_info
    {
        uint32_t algo_crc;
        uint32_t algo_len;
    };
}

class firmware_manager
{
public:
    firmware_manager() = default;
    esp_err_t init(const char *path);
    esp_err_t get_algo_name(char *algo_name, size_t len) const;
    esp_err_t get_target_name(char *target_name, size_t len) const;
    esp_err_t get_algo_bin(uint8_t *algo, size_t len) const;
    esp_err_t get_algo_bin_len(uint32_t &out) const;
    esp_err_t get_ram_size_byte(uint32_t &out) const;
    esp_err_t get_flash_size_byte(uint32_t &out) const;
    esp_err_t get_pc_init(uint32_t &out) const;
    esp_err_t get_pc_uninit(uint32_t &out) const;
    esp_err_t get_pc_program_page(uint32_t &out) const;
    esp_err_t get_pc_erase_sector(uint32_t &out) const;
    esp_err_t get_pc_erase_all(uint32_t &out) const;
    esp_err_t get_data_section_offset(uint32_t &out) const;
    esp_err_t get_flash_start_addr(uint32_t &out) const;
    esp_err_t get_flash_end_addr(uint32_t &out) const;
    esp_err_t get_page_size(uint32_t &out) const;
    esp_err_t get_erased_byte_val(uint32_t &out) const;
    esp_err_t get_program_page_timeout(uint32_t &out) const;
    esp_err_t get_erase_sector_timeout(uint32_t &out) const;
    esp_err_t get_sector_size(uint32_t &out) const;

    esp_err_t set_algo_name(const char *algo_name);
    esp_err_t set_target_name(const char *target_name);
    esp_err_t set_algo_bin(const uint8_t *algo, size_t len);
    esp_err_t set_algo_bin_len(uint32_t value);
    esp_err_t set_ram_size_byte(uint32_t value);
    esp_err_t set_flash_size_byte(uint32_t value);
    esp_err_t set_pc_init(uint32_t value);
    esp_err_t set_pc_uninit(uint32_t value);
    esp_err_t set_pc_program_page(uint32_t value);
    esp_err_t set_pc_erase_sector(uint32_t value);
    esp_err_t set_pc_erase_all(uint32_t value);
    esp_err_t set_data_section_offset(uint32_t value);
    esp_err_t set_flash_start_addr(uint32_t value);
    esp_err_t set_flash_end_addr(uint32_t value);
    esp_err_t set_page_size(uint32_t value);
    esp_err_t set_erased_byte_val(uint32_t value);
    esp_err_t set_program_page_timeout(uint32_t value);
    esp_err_t set_erase_sector_timeout(uint32_t value);
    esp_err_t set_sector_size(uint32_t value);

    esp_err_t save_cfg(const uint8_t *buf, size_t len);
    esp_err_t read_cfg(uint8_t *out, size_t len) const;

    esp_err_t save_algo(const uint8_t *buf, size_t len);
    esp_err_t read_algo_info(uint8_t *out, size_t len) const;

private:
    static const constexpr char *TAG = "fw_mgr";
    std::shared_ptr<nvs::NVSHandle> nvs = soul_nvs::instance().nvs();
};
