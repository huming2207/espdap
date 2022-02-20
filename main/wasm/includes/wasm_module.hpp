#pragma once

#include <esp_err.h>
#include <cstdint>

#define RET_OK 0

#ifndef REG_NATIVE_FUNC
#define REG_NATIVE_FUNC(func_name, signature) \
    { #func_name, func_name##_wrapper, signature, NULL }
#endif

class wasm_module
{
public:
    virtual esp_err_t init() = 0;
    virtual esp_err_t deinit() = 0;
    virtual const char *name() const = 0;
    virtual size_t func_count() const = 0;
};

