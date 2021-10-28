#pragma once

#include <tinyusb.h>
#include <tusb_cdc_acm.h>
#include <esp_err.h>

class cdc_acm
{
public:
    static cdc_acm& instance()
    {
        static cdc_acm instance;
        return instance;
    }

    cdc_acm(cdc_acm const &) = delete;
    void operator=(cdc_acm const &) = delete;

private:
    cdc_acm() = default;
    esp_err_t init();
    static void serial_rx_cb(int itf, cdcacm_event_t *event);

private:
    static const constexpr char *TAG = "cdc_acm";
};

