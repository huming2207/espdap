#include <esp_spiffs.h>
#include <esp_log.h>
#include <mpack.h>
#include "manifest_mgr.hpp"


esp_err_t manifest_mgr::init()
{
    esp_vfs_spiffs_conf_t spiffs_config = {};
    spiffs_config.base_path = BASE_PATH;
    spiffs_config.format_if_mount_failed = false;
    spiffs_config.max_files = 3;
    spiffs_config.partition_label = nullptr;

    auto ret = esp_vfs_spiffs_register(&spiffs_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS: %s", esp_err_to_name(ret));
        return ret;
    }

    mpack_tree_t tree = {};
    mpack_tree_init_filename(&tree, MANIFEST_PATH, 3072);
    if (mpack_tree_error(&tree) != mpack_ok) {
        ESP_LOGE(TAG, "Msgpack node tree failed to init");
        mpack_tree_destroy(&tree);
        return ESP_ERR_NO_MEM;
    }

    mpack_tree_parse(&tree);
    mpack_node_t root = mpack_tree_root(&tree);

    auto root_arr_len = mpack_node_array_length(root);
    if (root_arr_len == 0) {
        ESP_LOGE(TAG, "Nothing provided in the manifest");
        mpack_tree_destroy(&tree);
        return ESP_ERR_NOT_FOUND;
    }

    for (size_t idx = 0; idx < root_arr_len; idx += 1) {
        ret = parse_manifest_item(root, idx);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed when decoding manifest @ idx = %u", idx);
            mpack_tree_destroy(&tree);
            return ret;
        }
    }

    return ret;
}

esp_err_t manifest_mgr::parse_manifest_item(mpack_node_t node, size_t idx)
{
    if (manifest_cnt >= (sizeof(manifests) / sizeof(manifest_info))) {
        ESP_LOGE(TAG, "Manifest list is full!");
        return ESP_ERR_NO_MEM;
    }

    auto item_node = mpack_node_array_at(node, idx);
    auto fw_checksum = mpack_node_u32(mpack_node_map_cstr(item_node, "fw_crc"));
    auto alg_checksum = mpack_node_u32(mpack_node_map_cstr(item_node, "alg_crc"));
    if (alg_checksum == 0 || fw_checksum == 0) {
        ESP_LOGE(TAG, "Checksum info not found");
        return ESP_ERR_NOT_FOUND;
    }

    auto idcode = mpack_node_u32(mpack_node_map_cstr(item_node, "idcode"));
    if (idcode == 0) {
        ESP_LOGE(TAG, "IDCODE not found");
        return ESP_ERR_NOT_FOUND;
    }

    auto fw_name_node = mpack_node_map_cstr(item_node, "fw_name");
    auto fw_name_len = mpack_node_strlen(fw_name_node);
    if (fw_name_len < 1 || fw_name_len > 127) {
        ESP_LOGE(TAG, "Invalid firmware file name size: %u", fw_name_len);
        return ESP_ERR_INVALID_SIZE;
    }

    char fw_name[128] = { 0 };
    mpack_node_copy_cstr(fw_name_node, fw_name, sizeof(fw_name));

    auto alg_name_node = mpack_node_map_cstr(item_node, "alg_name");
    auto alg_name_len = mpack_node_strlen(alg_name_node);
    if (alg_name_len < 1 || alg_name_len > 127) {
        ESP_LOGE(TAG, "Invalid flash algorithm file name size: %u", alg_name_len);
        return ESP_ERR_INVALID_SIZE;
    }

    char alg_name[128] = { 0 };
    mpack_node_copy_cstr(alg_name_node, alg_name, sizeof(alg_name));

    manifest_cnt += 1;
    manifests[manifest_cnt - 1].fw_name = strdup(fw_name);
    manifests[manifest_cnt - 1].algo_name = strdup(alg_name);
    manifests[manifest_cnt - 1].fw_checksum = fw_checksum;
    manifests[manifest_cnt - 1].idcode = idcode;
    manifests[manifest_cnt - 1].algo_checksum = alg_checksum;

    return ESP_OK;
}

manifest_info *manifest_mgr::get_manifests()
{
    return manifests;
}

uint8_t manifest_mgr::get_manifest_count() const
{
    return manifest_cnt;
}
