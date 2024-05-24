#pragma once

#include <esp_err.h>
#include <multi_heap.h>
#include "rpc_packet.hpp"
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

private:
    mq_client() = default;

public:
    esp_err_t init(esp_mqtt_client_config_t *_mqtt_cfg);

public:
    esp_err_t pair();
    esp_err_t login();

public:
    esp_err_t record_erase(rpc::erase_event *erase_evt);
    esp_err_t record_program(rpc::prog_event *prog_evt);
    esp_err_t record_self_test(rpc::self_test_event *test_evt, uint8_t *result_payload, size_t payload_len);
    esp_err_t record_repair(rpc::repair_event *repair_evt);
    esp_err_t record_dispose(rpc::repair_event *repair_evt);

private:
    esp_mqtt_client_handle_t mqtt_handle = nullptr;
    esp_mqtt_client_config_t mqtt_cfg = {};
    char host_sn[16] = {};

private:
    static void mq_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
    esp_err_t record_stuff(rpc::base_event *event, const char *event_subtopic);

private:
    static const constexpr char TAG[] = "si_mqtt";
    static const constexpr char TOPIC_BASE[] = "soulinj/v1/prod";
    static const constexpr char TOPIC_PROG[] = "prog";
    static const constexpr char TOPIC_SELF_TEST[] = "test/int";
    static const constexpr char TOPIC_EXTN_TEST[] = "test/ext";
    static const constexpr char TOPIC_ERASE[] = "erase";
    static const constexpr char TOPIC_REPAIR[] = "repair";
    static const constexpr char TOPIC_DISPOSE[] = "dispose";
};