#include <cstring>
#include <esp_log.h>
#include <esp_crc.h>

#include "firmware_manager.hpp"

esp_err_t firmware_manager::init()
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

esp_err_t firmware_manager::save_cfg(const uint8_t *buf, size_t len)
{
    if (len < sizeof(fw_def::flash_algo_cfg)) {
        ESP_LOGE(TAG, "Incoming data too short");
        return ESP_ERR_INVALID_ARG;
    }

    if (buf == nullptr) {
        ESP_LOGE(TAG, "Incoming buffer is null");
        return ESP_ERR_INVALID_ARG;
    }

    auto *algo_cfg = (fw_def::flash_algo_cfg *)buf;

    // Magic is "JHSI" (Jackson Hu's Soul Injector firmware programmer)
    if (algo_cfg->magic != FW_MGR_MAGIC_NUMBER) {
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
    ret = ret ?: set_target_name(algo_cfg->target);

    return ret;
}

esp_err_t firmware_manager::read_cfg(uint8_t *out, size_t len) const
{
    if (len < sizeof(fw_def::flash_algo_cfg)) {
        ESP_LOGE(TAG, "Incoming buffer too short");
        return ESP_ERR_INVALID_ARG;
    }

    if (out == nullptr) {
        ESP_LOGE(TAG, "Incoming buffer is null");
        return ESP_ERR_INVALID_ARG;
    }

    auto *algo_cfg = (fw_def::flash_algo_cfg *)out;
    algo_cfg->magic = FW_MGR_MAGIC_NUMBER;

    char target[32] = { 0 };
    auto ret = get_target_name(target, 32);
    memcpy(algo_cfg->target, target, 32);

    char name[32] = { 0 };
    ret = ret ?: get_algo_name(name, 32);
    memcpy(algo_cfg->name, name, 32);

    uint32_t value = 0;
    ret = ret ?: get_pc_init(value);
    algo_cfg->pc_init = value;

    ret = ret ?: get_pc_uninit(value);
    algo_cfg->pc_uninit = value;

    ret = ret ?: get_pc_program_page(value);
    algo_cfg->pc_program_page = value;

    ret = ret ?: get_pc_erase_sector(value);
    algo_cfg->pc_erase_sector = value;

    ret = ret ?: get_pc_erase_all(value);
    algo_cfg->pc_erase_all = value;

    ret = ret ?: get_data_section_offset(value);
    algo_cfg->data_section_offset = value;

    ret = ret ?: get_flash_start_addr(value);
    algo_cfg->flash_start_addr = value;

    ret = ret ?: get_flash_end_addr(value);
    algo_cfg->flash_end_addr = value;

    ret = ret ?: get_page_size(value);
    algo_cfg->flash_page_size = value;

    ret = ret ?: get_sector_size(value);
    algo_cfg->flash_sector_size = value;

    ret = ret ?: get_program_page_timeout(value);
    algo_cfg->program_timeout = value;

    ret = ret ?: get_erased_byte_val(value);
    algo_cfg->erased_byte = value;

    ret = ret ?: get_erase_sector_timeout(value);
    algo_cfg->erase_timeout = value;

    ret = ret ?: get_ram_size_byte(value);
    algo_cfg->ram_size = value;

    ret = ret ?: get_flash_size_byte(value);
    algo_cfg->flash_size = value;

    return ret;
}

esp_err_t firmware_manager::save_algo(const uint8_t *buf, size_t len)
{
    if (buf == nullptr) {
        ESP_LOGE(TAG, "Incoming buffer is null");
        return ESP_ERR_INVALID_ARG;
    }

    if (len < 4) {
        ESP_LOGE(TAG, "Incoming buffer is too short");
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t expect_len = ((buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
    if (expect_len < len) {
        ESP_LOGE(TAG, "Incoming buffer is too short");
        return ESP_ERR_INVALID_ARG;
    }

    auto ret = set_algo_bin_len(expect_len);
    ret = ret ?: set_algo_bin(buf + sizeof(uint32_t), expect_len);

    return ret;
}

esp_err_t firmware_manager::read_algo_info(uint8_t *out, size_t len) const
{
    if (out == nullptr) {
        ESP_LOGE(TAG, "Incoming buffer is null");
        return ESP_ERR_INVALID_ARG;
    }

    if (len < sizeof(fw_def::algo_info)) {
        ESP_LOGE(TAG, "Incoming buffer is too short");
        return ESP_ERR_INVALID_ARG;
    }

    auto *info = (fw_def::algo_info *)out;
    uint32_t algo_len = 0;
    auto ret = get_algo_bin_len(algo_len);
    if (ret != ESP_OK || algo_len < 1) {
        info->algo_len = 0;
        info->algo_crc = 0;
        ESP_LOGW(TAG, "No algo stored");
    } else {
        info->algo_len = algo_len;
        auto *algo_buf = (uint8_t *)malloc(algo_len);
        if (algo_buf == nullptr) {
            info->algo_len = 0;
            info->algo_crc = 0;

            ESP_LOGE(TAG, "Memory alloc %u bytes failed when reading algo", algo_len);
            return ESP_ERR_NO_MEM;
        }

        ret = get_algo_bin(algo_buf, algo_len);
        if (ret != ESP_OK) {
            free(algo_buf);
            info->algo_len = 0;
            info->algo_crc = 0;

            ESP_LOGE(TAG, "Get algo blob failed");
            return ret;
        } else {
            info->algo_crc = esp_crc32_le(0, algo_buf, algo_len);
            info->algo_len = algo_len;
        }
    }

    return ret;
}

esp_err_t firmware_manager::load_default_cfg()
{
    fw_def::flash_algo_cfg cfg = {};

    // Default setting is for
    cfg.magic = FW_MGR_MAGIC_NUMBER;
    cfg.pc_init = 1;
    cfg.pc_uninit = 125;
    cfg.pc_program_page = 305;
    cfg.pc_erase_sector = 173;
    cfg.pc_erase_all = 0;
    cfg.data_section_offset = 512;
    cfg.flash_start_addr = 134217728;
    cfg.flash_end_addr = 134348800;
    cfg.flash_page_size = 1024;
    cfg.erased_byte = 0;
    cfg.flash_sector_size = 128;
    cfg.program_timeout = 500;
    cfg.erase_timeout = 500;
    cfg.ram_size = 20480;
    cfg.flash_size = 131072;

    return save_cfg((const uint8_t *)&cfg, sizeof(cfg));
}

bool firmware_manager::has_valid_cfg() const
{
    uint8_t has_valid_cfg = 0;
    if (nvs->get_item("has_cfg", has_valid_cfg) != ESP_OK) {
        return false;
    }

    return has_valid_cfg > 0;
}

esp_err_t firmware_manager::set_has_cfg_flag(bool has_cfg)
{
    uint8_t val = has_cfg ? 1 : 0;
    return nvs->set_item("has_cfg", val);
}

esp_err_t firmware_manager::set_fw_crc(uint32_t crc)
{
    return nvs->set_item("fw_crc", crc);
}

esp_err_t firmware_manager::get_fw_crc(uint32_t &out) const
{
    return nvs->get_item("fw_crc", out);
}

