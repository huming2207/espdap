#pragma once

#define ARDUINOJSON_ENABLE_STRING_VIEW 1
#include <ArduinoJson.hpp>
#include <esp_wifi_types.h>
#include "psram_json_allocator.hpp"

namespace config
{
    struct wifi_cred
    {
        char ssid[32];
        char password[64];
    };

    struct mqtt_cred
    {
        std::string url;
        std::string username;
        std::string password;
        std::string client_id;
    };
}

class config_reader
{
public:
    static config_reader *instance()
    {
        static config_reader _instance;
        return &_instance;
    }

    void operator=(config_reader const &) = delete;
    config_reader(config_reader const &) = delete;

public:
    esp_err_t load();
    esp_err_t get_wifi_cred(wifi_config_t *cred);
    esp_err_t get_mqtt_cred(config::mqtt_cred &mq_cred);
    esp_err_t get_mac_addr(uint8_t *mac_addr);
    esp_err_t reload_config();
    [[nodiscard]] uint64_t get_flash_sn() const;

private:
    config_reader() = default;
    uint8_t mac_addr[6] = {};
    uint64_t flash_sn = 0;
    char full_sn[32] = {};
    json_file_reader cfg_reader {};
    PsRamAllocator json_allocator {};
    ArduinoJson::JsonDocument json_doc = ArduinoJson::JsonDocument(&json_allocator);

private:
    static const constexpr char TAG[] = "cfg_ldr";
    static const constexpr char CFG_FILE[] = "/data/config.json";
};
