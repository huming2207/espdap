#pragma once

#include <esp_err.h>

class wasm_module
{
public:
    virtual esp_err_t init() = 0;
    virtual esp_err_t deinit() = 0;
    virtual const char *name() const = 0;
    virtual size_t func_count() const = 0;
};