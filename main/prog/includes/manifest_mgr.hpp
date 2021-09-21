#pragma once

#include <esp_err.h>
#include <ArduinoJson.hpp>

#define MANIFEST_SIZE_BYTES 3072
#define MANIFEST_SIZE_JSON_DOC 4096

struct manifest_info
{
    uint32_t algo_checksum;
    uint32_t fw_checksum;
    const char *algo_name;
    const char *fw_name;
};

class manifest_mgr
{
public:
    static manifest_mgr& instance()
    {
        static manifest_mgr instance;
        return instance;
    }

    manifest_mgr(manifest_mgr const &) = delete;
    void operator=(manifest_mgr const &) = delete;

private:
    manifest_mgr() = default;
    esp_err_t parse_manifest_item(ArduinoJson::JsonVariantConst item);


private:
    static const constexpr char *TAG = "manifest_mgr";
    static const constexpr char *BASE_PATH = "/soul";
    static const constexpr char *MANIFEST_PATH = "/soul/manifest.json";

private:
    uint8_t manifest_cnt = 0;
    manifest_info manifests[8] = {};
    char *manifest_buf = new char[MANIFEST_SIZE_BYTES];
    ArduinoJson::DynamicJsonDocument json_doc = ArduinoJson::DynamicJsonDocument(MANIFEST_SIZE_JSON_DOC);

public:
    esp_err_t init();
    manifest_info *get_manifests();
    uint8_t get_manifest_count() const;
};

