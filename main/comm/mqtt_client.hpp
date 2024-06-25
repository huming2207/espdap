#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <esp_err.h>
#include <multi_heap.h>
#include <esp_log.h>
#include "rpc_report_packet.hpp"
#include "mqtt_client.h"


class mqtt_client
{
private:
    static const constexpr char TAG[] = "si_mqtt";

public:
    enum mqtt_states : uint32_t {
        MQ_STATE_FORCE_DISCONNECT = BIT(0),
        MQ_STATE_CONNECTED = BIT(1),
        MQ_STATE_SUBSCRIBED = BIT(2),
        MQ_STATE_REGISTERED = (MQ_STATE_CONNECTED | MQ_STATE_SUBSCRIBED),
        MQ_STATE_BIN_REQ_READY = BIT(3),
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
        size_t payload_len;
        uint8_t *blob;
        uint8_t buf[128]; // For small buffer, to avoid too much small alloc ops
    };

    static esp_err_t make_mq_cmd_packet(mq_cmd_pkt *pkt_out, cmd_type type, uint8_t *buf, size_t buf_len)
    {
        if (pkt_out == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }

        pkt_out->type = type;

        if (buf == nullptr || buf_len < 1) {
            return ESP_OK;
        }

        if (buf_len > sizeof(mq_cmd_pkt::buf)) {
            pkt_out->blob = (uint8_t *)heap_caps_calloc(1, buf_len, MALLOC_CAP_SPIRAM);
            if (pkt_out->blob == nullptr) {
                ESP_LOGE(TAG, "Failed to alloc BLOB in cmd packet, len=%u", buf_len);
                return ESP_ERR_NO_MEM;
            }
        } else {
            memcpy(pkt_out->buf, buf, buf_len);
        }

        pkt_out->payload_len = buf_len;
        return ESP_OK;
    };

public:
    esp_err_t init(esp_mqtt_client_config_t *_mqtt_cfg);

public:
    esp_err_t connect();
    esp_err_t disconnect();

public:
    esp_err_t report_init(rpc::report::init_event *init_evt);
    esp_err_t report_host_state(rpc::report::state_event *state_evt);
    esp_err_t report_host_state(const char *msg, esp_err_t ret);
    esp_err_t report_erase(rpc::report::erase_event *erase_evt);
    esp_err_t report_program(rpc::report::prog_event *prog_evt);
    esp_err_t report_self_test(rpc::report::self_test_event *test_evt, uint8_t *result_payload, size_t payload_len);
    esp_err_t report_repair(rpc::report::repair_event *repair_evt);
    esp_err_t report_dispose(rpc::report::repair_event *repair_evt);
    esp_err_t recv_cmd_packet(mq_cmd_pkt *cmd_pkt, uint32_t timeout_ticks = portMAX_DELAY);
    esp_err_t request_blob(const char *type, uint32_t offset, size_t expect_blk_len, uint32_t timeout_ticks = portMAX_DELAY);

public:
    esp_err_t subscribe_on_connect();

private:
    EventGroupHandle_t mqtt_state = nullptr;
    esp_mqtt_client_handle_t mqtt_handle = nullptr;
    esp_mqtt_client_config_t mqtt_cfg = {};
    QueueHandle_t cmd_queue = nullptr;
    char *request_blob_type = nullptr;
    uint32_t request_blob_offset = 0;
    size_t request_blob_max_len = 0;
    uint8_t host_sn[6] = {};

private:
    static void mq_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
    esp_err_t report_stuff(rpc::report::base_event *event, const char *event_subtopic);
    esp_err_t decode_cmd_msg(const char *topic, size_t topic_len, uint8_t *buf, size_t buf_len);

};