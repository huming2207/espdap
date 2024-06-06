#include <multi_heap.h>
#include <mq_client.hpp>
#include <esp_efuse.h>
#include <esp_http_client.h>
#include <esp_mac.h>
#include "rpc_packet.hpp"

esp_err_t mq_client::init(esp_mqtt_client_config_t *_mqtt_cfg)
{
    memcpy(&mqtt_cfg, _mqtt_cfg, sizeof(esp_mqtt_client_config_t));
    mqtt_handle = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_handle != nullptr) {
        return ESP_FAIL;
    }

    return esp_mqtt_client_register_event(mqtt_handle, MQTT_EVENT_ANY, mq_event_handler, this);;
}

esp_err_t mq_client::pair()
{
    return 0;
}

esp_err_t mq_client::login()
{
    return 0;
}

esp_err_t mq_client::record_stuff(rpc::base_event *event, const char *event_subtopic)
{
    if (event_subtopic == nullptr) {
        ESP_LOGE(TAG, "record: invalid arg %p %p", event, event_subtopic);
        return ESP_ERR_INVALID_ARG;
    }

    if (strlen(event_subtopic) > 16) {
        ESP_LOGE(TAG, "Event subtopic too long: %u", strlen(event_subtopic));
        return ESP_ERR_NO_MEM;
    }

    char topic_full[sizeof(TOPIC_REPORT_BASE) + sizeof(host_sn) + 16] = {};
    snprintf(topic_full, sizeof(topic_full), "%s/" MACSTR "/%s", TOPIC_REPORT_BASE, MAC2STR(host_sn), event_subtopic);
    topic_full[sizeof(topic_full) - 1] = '\0';

    uint8_t msgpack_buf_stack[256] = {};
    uint8_t *msgpack_buf = nullptr;
    size_t expected_size = event->get_serialized_size();
    bool heap_allocated = expected_size > sizeof(msgpack_buf_stack);
    if (heap_allocated) {
        ESP_LOGW(TAG, "Alloc'ing msgpack buffer on heap, size=%u topic=%s", expected_size, event_subtopic);
        msgpack_buf = (uint8_t *) heap_caps_calloc(1, expected_size, MALLOC_CAP_SPIRAM);
        if (msgpack_buf == nullptr) {
            ESP_LOGE(TAG, "Heap alloc failed, size=%u topic=%s", expected_size, event_subtopic);
            return ESP_ERR_NO_MEM;
        }
    } else {
        msgpack_buf = msgpack_buf_stack;
    }

    size_t serialised_len = event->serialize(msgpack_buf, sizeof(msgpack_buf));
    int ret = esp_mqtt_client_enqueue(mqtt_handle, topic_full, (const char *)msgpack_buf, (int)serialised_len, 1, 1, true);

    if (heap_allocated) {
        free(msgpack_buf);
    }

    if (ret == -1) {
        ESP_LOGE(TAG, "record: failed to enqueue, dunno why");
        return ESP_FAIL;
    } else if (ret == -2) {
        ESP_LOGE(TAG, "record: MQTT queue full!");
        return ESP_ERR_NO_MEM; // -2 means queue is full
    }

    return ESP_OK;
}

esp_err_t mq_client::record_erase(rpc::erase_event *erase_evt)
{
    return record_stuff(erase_evt, TOPIC_REPORT_ERASE);
}

esp_err_t mq_client::record_program(rpc::prog_event *prog_evt)
{
    return record_stuff(prog_evt, TOPIC_REPORT_PROG);
}

esp_err_t mq_client::record_self_test(rpc::self_test_event *test_evt, uint8_t *result_payload, size_t payload_len)
{
    return record_stuff(test_evt, TOPIC_REPORT_SELF_TEST);
}

esp_err_t mq_client::record_repair(rpc::repair_event *repair_evt)
{
    return record_stuff(repair_evt, TOPIC_REPORT_REPAIR);
}

esp_err_t mq_client::record_dispose(rpc::repair_event *repair_evt)
{
    return record_stuff(repair_evt, TOPIC_REPORT_DISPOSE);
}

void mq_client::mq_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    auto *ctx = (mq_client *)handler_args;
    auto *mqtt_evt = (esp_mqtt_event_handle_t)event_data;

    if (ctx == nullptr || mqtt_evt == nullptr) {
        return;
    }

    switch (mqtt_evt->event_id) {
        case MQTT_EVENT_ERROR: {
            break;
        }
        case MQTT_EVENT_CONNECTED: {
            if (ctx->subscribe_on_connect() != ESP_OK) {
                ESP_LOGE(TAG, "Failed to subscribe, disconnect now!");
                esp_mqtt_client_disconnect(mqtt_evt->client);
            } else {
                ESP_LOGI(TAG, "Subscribe OK");
            }

            break;
        }

        case MQTT_EVENT_DISCONNECTED: {
            break;
        }
        case MQTT_EVENT_SUBSCRIBED: {
            break;
        }
        case MQTT_EVENT_UNSUBSCRIBED: {
            break;
        }
        case MQTT_EVENT_PUBLISHED: {
            break;
        }
        case MQTT_EVENT_DATA: {
            break;
        }
        case MQTT_EVENT_BEFORE_CONNECT: {
            break;
        }
        case MQTT_EVENT_DELETED: {
            break;
        }
        default: {
            break;
        }
    }
}

esp_err_t mq_client::subscribe_on_connect()
{
    char topic_str[sizeof(TOPIC_CMD_BASE) + sizeof(host_sn) + 16] = {};
    esp_mqtt_topic_t topics[4] = {};

    snprintf(topic_str, sizeof(topic_str), "%s/" MACSTR "/%s", TOPIC_CMD_BASE, MAC2STR(host_sn), TOPIC_CMD_READ_MEM);
    topic_str[sizeof(topic_str) - 1] = '\0';
    topics[0].filter = strdup((const char *)topics);
    topics[0].qos = 2;
    memset(topic_str, 0, sizeof(topic_str));

    snprintf(topic_str, sizeof(topic_str), "%s/" MACSTR "/%s", TOPIC_CMD_BASE, MAC2STR(host_sn), TOPIC_CMD_SET_STATE);
    topic_str[sizeof(topic_str) - 1] = '\0';
    topics[1].filter = strdup((const char *)topics);
    topics[1].qos = 2;
    memset(topic_str, 0, sizeof(topic_str));

    snprintf(topic_str, sizeof(topic_str), "%s/" MACSTR "/%s", TOPIC_CMD_BASE, MAC2STR(host_sn), TOPIC_CMD_BIN_FIRMWARE);
    topic_str[sizeof(topic_str) - 1] = '\0';
    topics[2].filter = strdup((const char *)topics);
    topics[2].qos = 2;
    memset(topic_str, 0, sizeof(topic_str));

    snprintf(topic_str, sizeof(topic_str), "%s/" MACSTR "/%s", TOPIC_CMD_BASE, MAC2STR(host_sn), TOPIC_CMD_BIN_FLASH_ALGO);
    topic_str[sizeof(topic_str) - 1] = '\0';
    topics[3].filter = strdup((const char *)topics);
    topics[3].qos = 2;
    memset(topic_str, 0, sizeof(topic_str));


    auto ret = esp_mqtt_client_subscribe_multiple(mqtt_handle, topics, 4);
    ESP_LOGI(TAG, "Subscribing to %s, ret=%d", topic_str, ret);

    for (size_t idx = 0; idx < 4; idx += 1) {
        if (topics[idx].filter != nullptr) {
            free((void *) topics[idx].filter);
            topics[idx].filter = nullptr;
        }
    }

    if (ret < 0) {
        if (ret == -1) {
            ESP_LOGE(TAG, "Failed to subscribe, unknown error");
            return ESP_FAIL;
        } else if (ret == -2) {
            ESP_LOGE(TAG, "Failed to subscribe, msgbox full");
            return ESP_ERR_NO_MEM;
        } else {
            ESP_LOGE(TAG, "Unknown state: %d", ret);
            return ESP_ERR_INVALID_STATE;
        }
    } else {
        return ESP_OK;
    }
}
