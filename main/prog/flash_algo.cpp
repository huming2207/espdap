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

    mp_buf = new uint8_t[len]; // Probably I should force it to be in PSRAM??
    memcpy(mp_buf, mp_file, len);
    mpack_tree_init(&mp_tree, (const char *)mp_buf, len);

    return ESP_OK;
}

flash_algo::~flash_algo()
{
    delete mp_buf;
    delete target_name;
    delete algo_name;
}

esp_err_t flash_algo::parse()
{
    mpack_node_t root = mpack_tree_root(&mp_tree);
    idcode = mpack_node_u32(mpack_node_map_cstr(root, "idcode"));
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
    memset(algo_name, 0, algo_name_len + 1);
    mpack_node_copy_cstr(algo_name_node, algo_name, algo_name_len);

    return ESP_OK;
}
