#pragma once

#include <esp_err.h>
#include "config_reader.hpp"
#include "wifi_manager.hpp"
#include "mqtt_client.hpp"

class bootstrap_fsm
{
public:
    static bootstrap_fsm *instance()
    {
        static bootstrap_fsm _instance;
        return &_instance;
    }

    bootstrap_fsm(bootstrap_fsm const &) = delete;
    void operator=(bootstrap_fsm const &) = delete;

private:
    bootstrap_fsm() = default;

public:
    esp_err_t init();
    esp_err_t handle_mqtt_cmd();

private:
    esp_err_t init_load_config();
    esp_err_t init_mq_client();
    esp_err_t init_connect_wifi();


private:
    config_reader *cfg_reader = config_reader::instance();
    wifi_manager wifi = {};
    mqtt_client mq_client = {};

private:
    static const constexpr char TAG[] = "bootstrap_fsm";
};

