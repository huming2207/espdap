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
    esp_err_t parse();

private:
    uint8_t sector_info_cnt = 0;
    uint8_t *mp_buf = nullptr;
    static const constexpr char *TAG = "flash_algo";
    char *algo_name = nullptr;
    char *target_name = nullptr;
    uint32_t idcode = 0;
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
    sector_info sector_info_arr[8] = {};

    mpack_tree_t mp_tree = {};
};
