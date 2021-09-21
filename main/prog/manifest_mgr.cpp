#include <esp_spiffs.h>
#include <esp_log.h>
#include <mpack.h>
#include <ArduinoJson.hpp>
#include "manifest_mgr.hpp"

using namespace ArduinoJson;

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

    FILE *file = fopen(MANIFEST_PATH, "r");
    if (file == nullptr) {
        ESP_LOGE(TAG, "Failed when reading file");
        return ESP_ERR_NOT_FOUND;
    }

    fseek(file, 0, SEEK_END);
    size_t len = ftell(file);
    if (len < 1 || len > MANIFEST_SIZE_BYTES - 1) {
        ESP_LOGE(TAG, "Manifest in a wrong length: %u", len);
        return ESP_ERR_INVALID_STATE;
    }

    fseek(file, 0, SEEK_SET);
    fread(manifest_buf, 1, len, file);
    fclose(file);

    if (deserializeJson(json_doc, manifest_buf, MANIFEST_SIZE_BYTES) != DeserializationError::Ok) {
        ESP_LOGE(TAG, "Failed to parse manifest JSON");
        return ESP_ERR_INVALID_STATE;
    }

    auto root_array = json_doc.as<JsonArrayConst>();
    for(auto item : root_array) {
        ret = parse_manifest_item(item);
    }

    return ret;
}

esp_err_t manifest_mgr::parse_manifest_item(JsonVariantConst item)
{
    if (manifest_cnt >= (sizeof(manifests) / sizeof(manifest_info))) {
        ESP_LOGE(TAG, "Manifest list is full!");
        return ESP_ERR_NO_MEM;
    }

    if (item.isNull() || item.isUndefined()) {
        ESP_LOGW(TAG, "Empty manifest entry found!");
        return ESP_ERR_NOT_FOUND;
    }


    auto fw_checksum = item["fwCrc"].as<uint32_t>();
    auto alg_checksum = item["algCrc"].as<uint32_t>();
    if (alg_checksum == 0 || fw_checksum == 0) {
        ESP_LOGE(TAG, "Checksum info not found");
        return ESP_ERR_NOT_FOUND;
    }

    const char *fw_name = item["fwName"].as<const char *>();
    size_t fw_name_len = strlen(fw_name);
    if (fw_name_len < 1 || fw_name_len > 127) {
        ESP_LOGE(TAG, "Invalid firmware file name size: %u", fw_name_len);
        return ESP_ERR_INVALID_SIZE;
    }

    const char *alg_name = item["fwName"].as<const char *>();
    size_t alg_name_len = strlen(fw_name);
    if (fw_name_len < 1 || fw_name_len > 127) {
        ESP_LOGE(TAG, "Invalid algorithm file name size: %u", alg_name_len);
        return ESP_ERR_INVALID_SIZE;
    }

    manifest_cnt += 1;
    manifests[manifest_cnt - 1].fw_name = strdup(fw_name);
    manifests[manifest_cnt - 1].algo_name = strdup(alg_name);
    manifests[manifest_cnt - 1].fw_checksum = fw_checksum;
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
