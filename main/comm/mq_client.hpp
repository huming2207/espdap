#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <esp_err.h>
#include <multi_heap.h>
#include "rpc_report_packet.hpp"
#include "mqtt_client.h"


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

    enum cmd_type : uint32_t {
        MQ_CMD_META_FW,
        MQ_CMD_META_ALGO,
        MQ_CMD_BIN_FW,
        MQ_CMD_BIN_ALGO,
        MQ_CMD_SET_STATE,
        MQ_CMD_READ_MEM,
    };

    struct __attribute__((packed)) mq_cmd_pkt {
        cmd_type type;
        uint8_t *blob;
        uint32_t state;
        size_t blob_len;
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
    esp_err_t report_error(rpc::report::error_event *error_evt);
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
    QueueHandle_t cmd_queue = nullptr;
    uint8_t host_sn[6] = {};

private:
    static void mq_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
    esp_err_t report_stuff(rpc::report::base_event *event, const char *event_subtopic);
    esp_err_t decode_cmd_msg(const char *topic, size_t topic_len, uint8_t *buf, size_t buf_len);

private:
    static const constexpr char TAG[] = "si_mqtt";
};