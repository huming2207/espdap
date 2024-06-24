#include <esp_event.h>
#include "bootstrap_fsm.hpp"
#include "mqtt_client.h"

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

    switch (pkt.type) {
        case mqtt_client::MQ_CMD_META_FW: {
            break;
        }

        case mqtt_client::MQ_CMD_META_ALGO: {
            break;
        }

        case mqtt_client::MQ_CMD_BIN_FW: {
            break;
        }

        case mqtt_client::MQ_CMD_BIN_ALGO: {
            break;
        }

        case mqtt_client::MQ_CMD_SET_STATE: {
            break;
        }

        case mqtt_client::MQ_CMD_READ_MEM: {
            break;
        }
    }

    return ret;
}

