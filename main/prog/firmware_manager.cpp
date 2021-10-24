#include <cstring>
#include <esp_log.h>

#include <mbedtls/base64.h>
#include "firmware_manager.hpp"

esp_err_t firmware_manager::init(const char *path)
{

    return ESP_OK;
}

esp_err_t firmware_manager::get_algo_name(char *algo_name, size_t len) const
{
    return nvs->get_string("name", algo_name, len);
}

esp_err_t firmware_manager::get_target_name(char *target_name, size_t len) const
{
    return nvs->get_string("target", target_name, len);
}

esp_err_t firmware_manager::get_algo_bin(uint8_t *algo, size_t len) const
{
    return nvs->get_blob("algo_bin", algo, len);
}

esp_err_t firmware_manager::get_algo_bin_len(uint32_t &out) const
{
    return nvs->get_item("algo_len", out);
}

esp_err_t firmware_manager::get_ram_size_byte(uint32_t &out) const
{
    return nvs->get_item("ram_size", out);
}

esp_err_t firmware_manager::get_flash_size_byte(uint32_t &out) const
{
    return nvs->get_item("flash_size", out);
}

esp_err_t firmware_manager::get_pc_init(uint32_t &out) const
{
    return nvs->get_item("pc_init", out);
}

esp_err_t firmware_manager::get_pc_uninit(uint32_t &out) const
{
    return nvs->get_item("pc_uninit", out);
}

esp_err_t firmware_manager::get_pc_program_page(uint32_t &out) const
{
    return nvs->get_item("pc_prog_page", out);
}

esp_err_t firmware_manager::get_pc_erase_sector(uint32_t &out) const
{
    return nvs->get_item("pc_erase_sector", out);
}

esp_err_t firmware_manager::get_pc_erase_all(uint32_t &out) const
{
    return nvs->get_item("pc_erase_all", out);
}

esp_err_t firmware_manager::get_data_section_offset(uint32_t &out) const
{
    return nvs->get_item("data_sc_offset", out);
}

esp_err_t firmware_manager::get_flash_start_addr(uint32_t &out) const
{
    return nvs->get_item("fl_start_addr", out);
}

esp_err_t firmware_manager::get_flash_end_addr(uint32_t &out) const
{
    return nvs->get_item("fl_end_addr", out);
}

esp_err_t firmware_manager::get_page_size(uint32_t &out) const
{
    return nvs->get_item("flash_page_size", out);
}

esp_err_t firmware_manager::get_erased_byte_val(uint32_t &out) const
{
    return nvs->get_item("erased_byte", out);
}

esp_err_t firmware_manager::get_program_page_timeout(uint32_t &out) const
{
    return nvs->get_item("prog_timeout", out);
}

esp_err_t firmware_manager::get_erase_sector_timeout(uint32_t &out) const
{
    return nvs->get_item("erase_timeout", out);
}

esp_err_t firmware_manager::get_sector_size(uint32_t &out) const
{
    return nvs->get_item("flash_sector_sz", out);
}

esp_err_t firmware_manager::set_algo_name(const char *algo_name)
{
    return nvs->set_string("name", algo_name);
}

esp_err_t firmware_manager::set_target_name(const char *target_name)
{
    return nvs->set_string("target", target_name);
}

esp_err_t firmware_manager::set_algo_bin(const uint8_t *algo, size_t len)
{
    return nvs->set_blob("algo_bin", algo, len);
}

esp_err_t firmware_manager::set_algo_bin_len(uint32_t value)
{
    return nvs->set_item("algo_len", value);
}

esp_err_t firmware_manager::set_ram_size_byte(uint32_t value)
{
    return nvs->set_item("ram_size", value);
}

esp_err_t firmware_manager::set_flash_size_byte(uint32_t value)
{
    return nvs->set_item("flash_size", value);
}

esp_err_t firmware_manager::set_pc_init(uint32_t value)
{
    return nvs->set_item("pc_init", value);
}

esp_err_t firmware_manager::set_pc_uninit(uint32_t value)
{
    return nvs->set_item("pc_uninit", value);
}

esp_err_t firmware_manager::set_pc_program_page(uint32_t value)
{
    return nvs->set_item("pc_prog_page", value);
}

esp_err_t firmware_manager::set_pc_erase_sector(uint32_t value)
{
    return nvs->set_item("pc_erase_sector", value);
}

esp_err_t firmware_manager::set_pc_erase_all(uint32_t value)
{
    return nvs->set_item("pc_erase_all", value);
}

esp_err_t firmware_manager::set_data_section_offset(uint32_t value)
{
    return nvs->set_item("data_sc_offset", value);
}

esp_err_t firmware_manager::set_flash_start_addr(uint32_t value)
{
    return nvs->set_item("fl_start_addr", value);
}

esp_err_t firmware_manager::set_flash_end_addr(uint32_t value)
{
    return nvs->set_item("fl_end_addr", value);
}

esp_err_t firmware_manager::set_page_size(uint32_t value)
{
    return nvs->set_item("flash_page_size", value);
}

esp_err_t firmware_manager::set_erased_byte_val(uint32_t value)
{
    return nvs->set_item("erased_byte", value);
}

esp_err_t firmware_manager::set_program_page_timeout(uint32_t value)
{
    return nvs->set_item("prog_timeout", value);
}

esp_err_t firmware_manager::set_erase_sector_timeout(uint32_t value)
{
    return nvs->set_item("erase_timeout", value);
}

esp_err_t firmware_manager::set_sector_size(uint32_t value)
{
    return nvs->set_item("flash_sector_sz", value);
}

esp_err_t firmware_manager::deserialize_cfg(const uint8_t *buf, size_t len)
{
    if (len < 64) {
        ESP_LOGE(TAG, "Incoming data too short");
        return ESP_ERR_INVALID_ARG;
    }

    auto *algo_cfg = (fw_def::flash_algo_cfg *)buf;

    // Magic is "JHSI" (Jackson Hu's Soul Injector firmware programmer)
    if (algo_cfg->magic != 0x4a485349) {
        ESP_LOGE(TAG, "Invalid magic, expect 0x4a485349 got 0x%x", algo_cfg->magic);
        return ESP_ERR_INVALID_ARG;
    }

    auto ret = set_pc_init(algo_cfg->pc_init);
    ret = ret ?: set_pc_uninit(algo_cfg->pc_uninit);
    ret = ret ?: set_pc_program_page(algo_cfg->pc_program_page);
    ret = ret ?: set_pc_erase_sector(algo_cfg->pc_erase_sector);
    ret = ret ?: set_pc_erase_all(algo_cfg->pc_erase_all);
    ret = ret ?: set_data_section_offset(algo_cfg->data_section_offset);
    ret = ret ?: set_flash_start_addr(algo_cfg->flash_start_addr);
    ret = ret ?: set_flash_end_addr(algo_cfg->flash_end_addr);
    ret = ret ?: set_page_size(algo_cfg->flash_page_size);
    ret = ret ?: set_sector_size(algo_cfg->flash_sector_size);
    ret = ret ?: set_program_page_timeout(algo_cfg->program_timeout);
    ret = ret ?: set_erased_byte_val(algo_cfg->erased_byte);
    ret = ret ?: set_erase_sector_timeout(algo_cfg->erase_timeout);
    ret = ret ?: set_ram_size_byte(algo_cfg->ram_size);
    ret = ret ?: set_flash_size_byte(algo_cfg->flash_size);
    ret = ret ?: set_algo_name(algo_cfg->name);
    ret = ret ?: set_target_name(algo_cfg->description);

    return ret;
}
