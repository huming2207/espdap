#pragma once

#include <esp_err.h>
#include "rpc_packet.hpp"

class rpc_client
{
public:
    static rpc_client *instance()
    {
        static rpc_client _instance;
        return &_instance;
    }

    rpc_client(rpc_client const &) = delete;
    void operator=(rpc_client const &) = delete;

private:
    rpc_client() = default;

public:
    esp_err_t init();

public:
    esp_err_t pair();
    esp_err_t login();

public:
    esp_err_t record_erase(rpc::erase_event *erase_evt);
    esp_err_t record_program(rpc::prog_event *prog_evt);
    esp_err_t record_self_test(rpc::self_test_event *test_evt, uint8_t *result_payload, size_t payload_len);
    esp_err_t record_repair(rpc::repair_event *repair_evt, char *comment, size_t comment_len);
    esp_err_t record_dispose(rpc::repair_event *repair_evt, char *comment, size_t comment_len);
};