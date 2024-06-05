#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <esp_err.h>

#include <py/obj.h>
#include <py/runtime.h>
#include <port/micropython_embed.h>

esp_err_t mpy_runtime_bootstrap(uint8_t *heap_ptr, size_t heap_size, const char *script);

#ifdef __cplusplus
}
#endif