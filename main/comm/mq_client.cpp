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

    char erase_topic[sizeof(TOPIC_BASE) + sizeof(TOPIC_ERASE) + sizeof(host_sn)] = {};
    snprintf(erase_topic, sizeof(erase_topic), "%s/" MACSTR "/%s", TOPIC_BASE, MAC2STR(host_sn), event_subtopic);
    erase_topic[sizeof(erase_topic) - 1] = '\0';

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
    int ret = esp_mqtt_client_enqueue(mqtt_handle, erase_topic, (const char *)msgpack_buf, (int)serialised_len, 1, 1, true);

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
    return record_stuff(erase_evt, TOPIC_ERASE);
}

esp_err_t mq_client::record_program(rpc::prog_event *prog_evt)
{
    return record_stuff(prog_evt, TOPIC_PROG);
}

esp_err_t mq_client::record_self_test(rpc::self_test_event *test_evt, uint8_t *result_payload, size_t payload_len)
{
    return record_stuff(test_evt, TOPIC_ERASE);
}

esp_err_t mq_client::record_repair(rpc::repair_event *repair_evt)
{
    return record_stuff(repair_evt, TOPIC_REPAIR);
}

esp_err_t mq_client::record_dispose(rpc::repair_event *repair_evt)
{
    return record_stuff(repair_evt, TOPIC_DISPOSE);
}

void mq_client::mq_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{

}
