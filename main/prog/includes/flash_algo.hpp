#pragma once

#include <cstdint>
#include <cstddef>
#include <esp_err.h>
#include <mpack.h>

#define FLASH_ALGO_MAX_MP_SIZE 30720

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
    esp_err_t init(const uint8_t *mp_file, size_t len);
    char *get_algo_name() const;
    char *get_target_name() const;
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
    const sector_info &get_sector() const;

private:
    static const constexpr char *TAG = "flash_algo";
    char *algo_name = nullptr;
    char *target_name = nullptr;
    uint8_t *algo_bin = nullptr;
    uint32_t algo_bin_len = 0;
    uint32_t ram_size_byte = 0;
    uint32_t flash_size_byte = 0;
    uint32_t pc_init = 0;
    uint32_t pc_uninit = 0;
    uint32_t pc_program_page = 0;
    uint32_t pc_erase_sector = 0;
    uint32_t pc_erase_all = 0;
    uint32_t data_section_offset = 0;
    uint32_t flash_start_addr = 0;
    uint32_t flash_end_addr = 0;
    uint32_t page_size = 0;
    uint32_t erased_byte_val = 0;
    uint32_t program_page_timeout = 0;
    uint32_t erase_sector_timeout = 0;
    sector_info sector = {};

    mpack_tree_t mp_tree = {};
};
