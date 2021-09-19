#pragma once

#include <esp_err.h>
#include <mpack.h>

struct manifest_info
{
    uint32_t idcode;
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
    esp_err_t parse_manifest_item(mpack_node_t node, size_t idx);


private:
    static const constexpr char *TAG = "manifest";
    static const constexpr char *BASE_PATH = "/soul";
    static const constexpr char *MANIFEST_PATH = "/soul/manifest.bin";

private:
    uint8_t manifest_cnt = 0;
    manifest_info manifests[8] = {};

public:
    esp_err_t init();
    manifest_info *get_manifests();
    uint8_t get_manifest_count() const;
};

