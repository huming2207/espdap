#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_err.h>
#include <multi_heap.h>
#include "rpc_report_packet.hpp"
#include "mqtt_client.h"

#define static_char static const constexpr char

class mq_client
{
public:
    static mq_client *instance()
    {
        static mq_client _instance;
        return &_instance;
    }

    mq_client(mq_client const &) = delete;
    void operator=(mq_client const &) = delete;

public:
    enum mqtt_states : uint32_t {
        MQ_STATE_FORCE_DISCONNECT = BIT(0),
        MQ_STATE_CONNECTED = BIT(1),
        MQ_STATE_SUBSCRIBED = BIT(2),
        MQ_STATE_REGISTERED = (MQ_STATE_CONNECTED | MQ_STATE_SUBSCRIBED),
    };

private:
    mq_client() = default;

public:
    esp_err_t init(esp_mqtt_client_config_t *_mqtt_cfg);

public:
    esp_err_t connect();
    esp_err_t disconnect();

public:
    esp_err_t report_init(rpc::report::init_event *init_evt);
    esp_err_t report_erase(rpc::report::erase_event *erase_evt);
    esp_err_t report_program(rpc::report::prog_event *prog_evt);
    esp_err_t report_self_test(rpc::report::self_test_event *test_evt, uint8_t *result_payload, size_t payload_len);
    esp_err_t report_repair(rpc::report::repair_event *repair_evt);
    esp_err_t report_dispose(rpc::report::repair_event *repair_evt);

public:
    esp_err_t subscribe_on_connect();

private:
    EventGroupHandle_t mqtt_state = nullptr;
    esp_mqtt_client_handle_t mqtt_handle = nullptr;
    esp_mqtt_client_config_t mqtt_cfg = {};
    char host_sn[16] = {};

private:
    static void mq_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
    esp_err_t record_stuff(rpc::report::base_event *event, const char *event_subtopic);

private:
    static_char TAG[] = "si_mqtt";
    static_char TOPIC_REPORT_BASE[] = "/soulinjector/v1/report";
    static_char TOPIC_REPORT_INIT[] = "init";
    static_char TOPIC_REPORT_PROG[] = "prog";
    static_char TOPIC_REPORT_SELF_TEST[] = "test/int";
    static_char TOPIC_REPORT_EXTN_TEST[] = "test/ext";
    static_char TOPIC_REPORT_ERASE[] = "erase";
    static_char TOPIC_REPORT_REPAIR[] = "repair";
    static_char TOPIC_REPORT_DISPOSE[] = "dispose";

public:
    static_char TOPIC_CMD_BASE[] = "/soulinjector/v1/cmd";
    static_char TOPIC_CMD_METADATA_FIRMWARE[] = "meta/fw";
    static_char TOPIC_CMD_METADATA_FLASH_ALGO[] = "meta/algo";
    static_char TOPIC_CMD_BIN_FIRMWARE[] = "bin/fw";
    static_char TOPIC_CMD_BIN_FLASH_ALGO[] = "bin/algo";
    static_char TOPIC_CMD_SET_STATE[] = "state";
    static_char TOPIC_CMD_READ_MEM[] = "read_mem";
};