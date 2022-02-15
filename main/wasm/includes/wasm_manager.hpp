#pragma once

#include <esp_err.h>
#include "wasm_export.h"

class wasm_manager
{
public:
    static wasm_manager& instance()
    {
        static wasm_manager instance;
        return instance;
    }

    wasm_manager(wasm_manager const &) = delete;
    void operator=(wasm_manager const &) = delete;

private:
    wasm_manager() = default;

public:
    esp_err_t init();
    esp_err_t deinit();
    esp_err_t load(const uint8_t *buf, size_t len);
    esp_err_t unload();
    esp_err_t exec_main();

private:
    uint8_t *global_heap = nullptr;
    wasm_module_t mod = nullptr;
    wasm_module_inst_t mod_inst = nullptr;
    static const constexpr char *TAG = "wasm_mgr";
    RuntimeInitArgs init_args = {};
};

