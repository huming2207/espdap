#include <cstring>
#include <esp_log.h>

#include <mpack.h>
#include "flash_algo.hpp"

esp_err_t flash_algo::init(const uint8_t *mp_file, size_t len)
{
    if (len > FLASH_ALGO_MAX_MP_SIZE) {
        ESP_LOGE(TAG, "Msgpack size too large");
        return ESP_ERR_INVALID_SIZE;
    }

    if (mp_file == nullptr) {
        ESP_LOGE(TAG, "Msgpack buffer is null?");
        return ESP_ERR_INVALID_ARG;
    }

    mpack_tree_init(&mp_tree, (const char *) mp_file, len);
    if (mpack_tree_error(&mp_tree) != mpack_ok) {
        ESP_LOGE(TAG, "Msgpack node tree failed to init");
        mpack_tree_destroy(&mp_tree);
        return ESP_ERR_NO_MEM;
    }

    mpack_node_t root = mpack_tree_root(&mp_tree);
    auto algo_node = mpack_node_map_cstr(root, "algo");
    algo_bin_len = mpack_node_bin_size(algo_node);
    if (algo_bin_len == 0) {
        ESP_LOGE(TAG, "Flash algo not found or corrupted!");
        return ESP_ERR_NOT_FOUND;
    }

    algo_bin = new uint8_t[algo_bin_len];
    if (algo_bin == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate!");
        return ESP_ERR_NO_MEM;
    }

    mpack_node_copy_data(algo_node, (char *) algo_bin, algo_bin_len);

    pc_init = mpack_node_u32(mpack_node_map_cstr(root, "pc_init"));
    pc_uninit = mpack_node_u32(mpack_node_map_cstr(root, "pc_uninit"));
    if (pc_init == 0 || pc_uninit == 0) {
        ESP_LOGE(TAG, "No init/uninit function found in the algorithm!");
        return ESP_ERR_NOT_FOUND;
    }

    pc_program_page = mpack_node_u32(mpack_node_map_cstr(root, "pc_program_page"));
    if (pc_program_page == 0) {
        ESP_LOGE(TAG, "No program page function found in the algorithm!");
        return ESP_ERR_NOT_FOUND;
    }

    pc_erase_sector = mpack_node_u32(mpack_node_map_cstr(root, "pc_erase_sector"));
    if (pc_program_page == 0) {
        ESP_LOGE(TAG, "No erase sector function found in the algorithm!");
        return ESP_ERR_NOT_FOUND;
    }

    pc_erase_all = mpack_node_u32(mpack_node_map_cstr(root, "pc_erase_all"));
    ram_size_byte = mpack_node_u32(mpack_node_map_cstr(root, "ram_size"));
    flash_size_byte = mpack_node_u32(mpack_node_map_cstr(root, "flash_size_byte"));
    flash_start_addr = mpack_node_u32(mpack_node_map_cstr(root, "flash_start_addr"));
    flash_end_addr = mpack_node_u32(mpack_node_map_cstr(root, "flash_end_addr"));
    if (ram_size_byte == 0 || flash_size_byte == 0 || flash_start_addr == 0 || flash_end_addr == 0) {
        ESP_LOGE(TAG, "Flash details not sufficient");
        return ESP_ERR_NOT_FOUND;
    }

    page_size = mpack_node_u32(mpack_node_map_cstr(root, "page_size"));
    erased_byte_val = mpack_node_u32(mpack_node_map_cstr(root, "erased_byte_val"));
    program_page_timeout = mpack_node_u32(mpack_node_map_cstr(root, "program_page_timeout"));
    erase_sector_timeout = mpack_node_u32(mpack_node_map_cstr(root, "erase_sector_timeout"));
    data_section_offset = mpack_node_u32(mpack_node_map_cstr(root, "data_section_offset"));

    // Target name
    auto target_name_node = mpack_node_map_cstr(root, "target_name");
    auto target_name_len = mpack_node_strlen(target_name_node);

    if (target_name_len < 1 || target_name_len > 255) {
        ESP_LOGE(TAG, "No target name given, or too long");
        return ESP_ERR_NOT_FOUND;
    }

    target_name = new char[target_name_len + 1];
    if (target_name == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate!");
        return ESP_ERR_NO_MEM;
    }

    memset(target_name, 0, target_name_len + 1);
    mpack_node_copy_cstr(target_name_node, target_name, target_name_len);

    // Algorithm name
    auto algo_name_node = mpack_node_map_cstr(root, "algo_name");
    auto algo_name_len = mpack_node_strlen(algo_name_node);

    if (algo_name_len < 1 || algo_name_len > 255) {
        ESP_LOGE(TAG, "No algorithm name given, or too long: ");
        return ESP_ERR_NOT_FOUND;
    }

    algo_name = new char[algo_name_len + 1];
    if (algo_name == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate!");
        return ESP_ERR_NO_MEM;
    }

    memset(algo_name, 0, algo_name_len + 1);
    mpack_node_copy_cstr(algo_name_node, algo_name, algo_name_len);

    sector.addr = mpack_node_u32(mpack_node_map_cstr(root, "sector_addr"));
    sector.size = mpack_node_u32(mpack_node_map_cstr(root, "sector_size"));
    if (sector.size == 0) {
        ESP_LOGE(TAG, "Invalid sector size");
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

flash_algo::~flash_algo()
{
    delete target_name;
    delete algo_name;
    delete algo_bin;
    mpack_tree_destroy(&mp_tree);
}

char *flash_algo::get_algo_name() const
{
    return algo_name;
}

char *flash_algo::get_target_name() const
{
    return target_name;
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
    return ram_size_byte;
}

uint32_t flash_algo::get_flash_size_byte() const
{
    return flash_size_byte;
}

uint32_t flash_algo::get_pc_init() const
{
    return pc_init;
}

uint32_t flash_algo::get_pc_uninit() const
{
    return pc_uninit;
}

uint32_t flash_algo::get_pc_program_page() const
{
    return pc_program_page;
}

uint32_t flash_algo::get_pc_erase_sector() const
{
    return pc_erase_sector;
}

uint32_t flash_algo::get_pc_erase_all() const
{
    return pc_erase_all;
}

uint32_t flash_algo::get_data_section_offset() const
{
    return data_section_offset;
}

uint32_t flash_algo::get_flash_start_addr() const
{
    return flash_start_addr;
}

uint32_t flash_algo::get_flash_end_addr() const
{
    return flash_end_addr;
}

uint32_t flash_algo::get_page_size() const
{
    return page_size;
}

uint32_t flash_algo::get_erased_byte_val() const
{
    return erased_byte_val;
}

uint32_t flash_algo::get_program_page_timeout() const
{
    return program_page_timeout;
}

uint32_t flash_algo::get_erase_sector_timeout() const
{
    return erase_sector_timeout;
}

const sector_info &flash_algo::get_sector() const
{
    return sector;
}

