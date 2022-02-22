#pragma once

#include "wasm_module.hpp"
#include <wasm_export.h>

class rgb_led_module : public wasm_module
{
public:
    esp_err_t init() override;
    esp_err_t deinit() override;
    const char *name() const override;
    size_t func_count() const override;

private:
    static void set_color(uint8_t r, uint8_t g, uint8_t b);
    constexpr const static NativeSymbol symbols[] = {
            {"set_color", (void *)(set_color), "iii", nullptr }
    };
};

