#pragma once

#include <esp_err.h>

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
    esp_err_t init_load_config();
    esp_err_t init_mq_client();
};

