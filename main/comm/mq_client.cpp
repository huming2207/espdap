#include <rpc_client.hpp>
#include <esp_http_client.h>
#include "rpc_packet.hpp"

esp_err_t rpc_client::init()
{
    return ESP_OK;
}

esp_err_t rpc_client::pair()
{
    return 0;
}

esp_err_t rpc_client::login()
{
    return 0;
}

esp_err_t rpc_client::record_erase(rpc::erase_event *erase_evt)
{
    return 0;
}

esp_err_t rpc_client::record_program(rpc::prog_event *prog_evt)
{
    return 0;
}

esp_err_t rpc_client::record_self_test(rpc::self_test_event *test_evt, uint8_t *result_payload, size_t payload_len)
{
    return 0;
}

esp_err_t rpc_client::record_repair(rpc::repair_event *repair_evt, char *comment, size_t comment_len)
{
    return 0;
}

esp_err_t rpc_client::record_dispose(rpc::repair_event *repair_evt, char *comment, size_t comment_len)
{
    return 0;
}
