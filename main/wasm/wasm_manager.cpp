#include <esp_heap_caps.h>
#include <esp_log.h>
#include "wasm_manager.hpp"

esp_err_t wasm_manager::init()
{
    ESP_LOGI(TAG, "Allocating heap");
    if (global_heap == nullptr) {
        global_heap = static_cast<uint8_t *>(heap_caps_malloc(1 * 1024 * 1024, MALLOC_CAP_SPIRAM));
        if (global_heap == nullptr) {
            ESP_LOGE(TAG, "Failed to allocate global heap for WAMR");
            return ESP_ERR_NO_MEM;
        }
    }

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap;
    init_args.mem_alloc_option.pool.heap_size = 1 * 1024 * 1024;
    init_args.native_module_name = "env";

    ESP_LOGI(TAG, "Pre-init");

    if (!wasm_runtime_full_init(&init_args)) {
        ESP_LOGE(TAG, "Failed to init WAMR");
    }


    ESP_LOGI(TAG, "Init done");

    return ESP_OK;
}

esp_err_t wasm_manager::deinit()
{
    auto ret = unload();
    wasm_runtime_destroy();
    if (global_heap != nullptr) {
        free(global_heap);
        global_heap = nullptr;
    }

    return ret;
}

esp_err_t wasm_manager::load(const uint8_t *buf, size_t len)
{
    ESP_LOGI(TAG, "Pre-load");

    char err_buf[128] = { 0 };
    mod = wasm_runtime_load(buf, len, err_buf, sizeof(err_buf));
    if (mod == nullptr) {
        ESP_LOGE(TAG, "Failed to load WASM blob, reason: %s", err_buf);
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Load done");

    mod_inst = wasm_runtime_instantiate(mod, 32*1024, 32*1024, err_buf, sizeof(err_buf));
    if (mod_inst == nullptr) {
        ESP_LOGE(TAG, "Failed to init WASM blob, reason: %s", err_buf);
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "All done!");
    return ESP_OK;
}

esp_err_t wasm_manager::exec_main()
{
    bool called = wasm_application_execute_main(mod_inst, 0, nullptr);
    if (!called) {
        ESP_LOGE(TAG, "Failed to execute main function");
        return ESP_ERR_NOT_FINISHED;
    }

    auto *exception = wasm_runtime_get_exception(mod_inst);

    if (exception != nullptr) {
        ESP_LOGE(TAG, "Got exception: %s", exception);
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

esp_err_t wasm_manager::unload()
{
    wasm_runtime_deinstantiate(mod_inst);
    wasm_runtime_unload(mod);

    return ESP_OK;
}
