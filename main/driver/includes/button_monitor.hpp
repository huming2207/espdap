#pragma once

#include <driver/gpio.h>

class button_monitor
{
public:
    button_monitor(gpio_num_t _pin, bool _active_high)
    {
        pin = _pin;
        active_high = _active_high;

        gpio_config_t config = {};
        config.intr_type = GPIO_INTR_ANYEDGE;
        config.mode = GPIO_MODE_INPUT;
        config.pull_down_en = _active_high ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
        config.pull_up_en = _active_high ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE;
        config.pin_bit_mask = (1ULL << pin);

        gpio_config(&config);




    }

    ~button_monitor()
    {

    }

private:
    gpio_num_t pin = GPIO_NUM_MAX;
    bool active_high = false;
};