#include <esp_event.h>
#include "bootstrap_fsm.hpp"
#include "mqtt_client.h"
#include "fw_asset_manager.hpp"
#include "http_downloader.hpp"

esp_err_t bootstrap_fsm::init_load_config()
{
    return cfg_reader->load();
}

esp_err_t bootstrap_fsm::init_connect_wifi()
{
    return wifi.init();
}

esp_err_t bootstrap_fsm::init_mq_client()
{
    esp_mqtt_client_config_t mqtt_cfg = {};
    config::mqtt_cred mqtt_cred = {};

    mqtt_cfg.credentials.username = mqtt_cred.username.c_str();
    mqtt_cfg.credentials.authentication.password = mqtt_cred.password.c_str();
    mqtt_cfg.credentials.set_null_client_id = false;
    if (!mqtt_cred.client_id.empty()) {
        mqtt_cfg.credentials.client_id = mqtt_cred.client_id.c_str();
    } else {
        char sn_str[32] = {};
        char default_client_id[64] = {};
        cfg_reader->get_full_sn_str(sn_str, sizeof(sn_str));
        snprintf(default_client_id, sizeof(default_client_id), "si-%s", sn_str);
    }

    return mq_client.init(&mqtt_cfg);
}

esp_err_t bootstrap_fsm::init()
{
    ESP_LOGI(TAG, "Connect WiFi...");
    auto ret = init_load_config();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load config: 0x%x %s", ret, esp_err_to_name(ret));
        // TODO handle load config failure here - stuck here forever??
    }

    ret = init_connect_wifi();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect WiFi: 0x%x %s", ret, esp_err_to_name(ret));
        // TODO handle wifi failure here - go to offline dumb mode?
    }

    ret = init_mq_client();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect: 0x%x %s", ret, esp_err_to_name(ret));
        // TODO handle wifi failure here - go to offline dumb mode?
    }


    return 0;
}

esp_err_t bootstrap_fsm::handle_mqtt_cmd()
{
    mqtt_client::mq_cmd_pkt pkt = {};
    auto ret = mq_client.recv_cmd_packet(&pkt);

    if (pkt.payload_len < 1) {
        return ESP_OK; // Ignore all packet with no payload for now??
    }

    PsRamAllocator allocator = {};
    auto json_doc = ArduinoJson::JsonDocument(&allocator);
    uint8_t *buf = pkt.payload_len > sizeof(mqtt_client::mq_cmd_pkt::buf) ? pkt.blob : pkt.buf;
    if (buf == nullptr) {
        ESP_LOGE(TAG, "Malformed cmd, type=%lu, len=%u; %p; %p", pkt.type, pkt.payload_len, pkt.buf, pkt.blob);
        return ESP_ERR_INVALID_STATE;
    }

    auto err = ArduinoJson::deserializeMsgPack(json_doc, buf, pkt.payload_len);
    if (err == ArduinoJson::DeserializationError::EmptyInput
        || err == ArduinoJson::DeserializationError::IncompleteInput || err == ArduinoJson::DeserializationError::InvalidInput) {
        ESP_LOGE(TAG, "Failed to decode CMD payload: %s", err.c_str());
        return ESP_ERR_NOT_SUPPORTED;
    } else if (err == ArduinoJson::DeserializationError::NoMemory || err == ArduinoJson::DeserializationError::TooDeep) {
        ESP_LOGE(TAG, "No memory to handle CMD payload: %s", err.c_str());
        return ESP_ERR_NO_MEM;
    }

    switch (pkt.type) {
        case mqtt_client::MQ_CMD_META_FW: {
            return decode_mqtt_cmd_meta_fw(json_doc);
        }

        case mqtt_client::MQ_CMD_META_ALGO: {
            return decode_mqtt_cmd_meta_algo(json_doc);
        }

        case mqtt_client::MQ_CMD_BIN_FW: {
            return decode_mqtt_cmd_bin_fw(json_doc);
        }

        case mqtt_client::MQ_CMD_BIN_ALGO: {
            return decode_mqtt_cmd_bin_algo(json_doc);
        }

        case mqtt_client::MQ_CMD_SET_STATE: {
            return decode_mqtt_cmd_set_state(json_doc);
        }

        case mqtt_client::MQ_CMD_READ_MEM: {
            return decode_mqtt_cmd_bin_algo(json_doc);
        }
    }

    return ret;
}

esp_err_t bootstrap_fsm::decode_mqtt_cmd_meta_fw(ArduinoJson::JsonDocument &doc)
{
    uint8_t hash_buf[32] = {};
    ArduinoJson::MsgPackBinary hash_bin = doc["hash"];
    if (hash_bin.data() == nullptr || hash_bin.size() < 32) {
        ESP_LOGE(TAG, "Invalid metadata hash");
        return ESP_ERR_INVALID_ARG;
    } else {
        memcpy(hash_buf, hash_bin.data(), sizeof(hash_buf));
    }

    if (doc.containsKey("url") && !fw_asset_manager::check_fw_bin_hash(hash_buf, sizeof(hash_buf))) {
        http_downloader downloader = {};
        auto ret = downloader.init(doc["url"], fw_asset_manager::FIRMWARE_PATH);
        ret = ret ?: downloader.request();
        if (ret != ESP_OK) {
            mq_client.report_host_state("Can't download firmware!", ret);
            ESP_LOGE(TAG, "Can't download firmware! 0x%x %s", ret, esp_err_to_name(ret));
            return ret;
        }
    } else {
        mq_client.report_host_state("Same firmware", ESP_OK);
    }

    return ESP_OK;
}

esp_err_t bootstrap_fsm::decode_mqtt_cmd_meta_algo(ArduinoJson::JsonDocument &doc)
{
    uint8_t hash_buf[32] = {};
    ArduinoJson::MsgPackBinary hash_bin = doc["hash"];
    if (hash_bin.data() == nullptr || hash_bin.size() < 32) {
        ESP_LOGE(TAG, "Invalid metadata hash");
        return ESP_ERR_INVALID_ARG;
    } else {
        memcpy(hash_buf, hash_bin.data(), sizeof(hash_buf));
    }

    if (doc.containsKey("url") && !fw_asset_manager::check_algo_bin_hash(hash_buf, sizeof(hash_buf))) {
        http_downloader downloader = {};
        auto ret = downloader.init(doc["url"], fw_asset_manager::ALGO_ELF_PATH);
        ret = ret ?: downloader.request();
        if (ret != ESP_OK) {
            mq_client.report_host_state("Can't download flash algo!", ret);
            ESP_LOGE(TAG, "Can't download flash algo! 0x%x %s", ret, esp_err_to_name(ret));
            return ret;
        }
    } else {
        mq_client.report_host_state("Same algo", ESP_OK);
    }

    return ESP_OK;
}

esp_err_t bootstrap_fsm::decode_mqtt_cmd_bin_fw(ArduinoJson::JsonDocument &doc)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bootstrap_fsm::decode_mqtt_cmd_bin_algo(ArduinoJson::JsonDocument &doc)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bootstrap_fsm::decode_mqtt_cmd_set_state(ArduinoJson::JsonDocument &doc)
{
    return 0;
}

esp_err_t bootstrap_fsm::decode_mqtt_cmd_read_mem(ArduinoJson::JsonDocument &doc)
{
    return 0;
}

