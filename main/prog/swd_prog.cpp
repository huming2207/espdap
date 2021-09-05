#include <swd_host.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <cstring>
#include <mbedtls/base64.h>
#include "swd_prog.hpp"

#define TAG "swd_prog"

// From: https://github.com/probe-rs/probe-rs/blob/master/probe-rs/targets/STM32L0_Series.yaml#L3997
const static constexpr char flash_algo[] = "ASoB0AIqF9E7SIFpDyISAhFDgWE5ScFgOUnBYDlJAWE5SQFhwGnAAgbUOUg3SQFgBiFBYDdJgWAAIHBHA"
                                           "SgB0AIoCNEsSEFoAiIRQ0FgQWgBIhFDQWAAIHBHMLUmSUpoTBUiQ0pgSmgIJSpDSmAAIgJgKUgmSgDgEGC"
                                           "LadsH+9FIaKBDSGBIaKhDSGAAIDC9ASBwR/C1GEwAIyUVCCY/MYkJjEYk4GFoKUNhYGFoMUNhYEAhgMqAwA"
                                           "kfACn60RZJp2n/BwLQEk85YPnnoWkJBQkPBtCgaQ8hCQIIQ6BhASDwvWFoqUNhYGFosUNhYFscnEXY2AAg"
                                           "8L0AIAJA782riQUEAwK/rp2MFhUUE1VVAAAAMABA/w8AAKqqAAAAAAAA";

const uint32_t swd_prog::header_blob[] = {
        0xE00ABE00,
        0x062D780D,
        0x24084068,
        0xD3000040,
        0x1E644058,
        0x1C49D1FA,
        0x2A001E52,
        0x04770D1F,
};

esp_err_t swd_prog::load_flash_algorithm()
{
    auto ret = swd_halt_target();
    if (ret < 1) {
        ESP_LOGE(TAG, "Failed when halting");
        state = swd_def::UNKNOWN;
        return ESP_ERR_INVALID_STATE;
    }

    ret = swd_wait_until_halted();
    if (ret < 1) {
        ESP_LOGE(TAG, "Timeout when halting");
        state = swd_def::UNKNOWN;
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t flash_algo_decoded[350] = { 0 };
    size_t flash_algo_len = 0;
    int mbedtls_ret = mbedtls_base64_decode(
                flash_algo_decoded, sizeof(flash_algo_decoded),
                &flash_algo_len,
                (uint8_t *)flash_algo, strlen(flash_algo)
            );

    ESP_LOGI(TAG, "Mbedtls returned %d, flash algo len %u, loading to 0x%x", mbedtls_ret, flash_algo_len, (uint32_t)(code_start + sizeof(header_blob)));
    ESP_LOG_BUFFER_HEX(TAG, header_blob, sizeof(header_blob));
    ESP_LOG_BUFFER_HEX(TAG, flash_algo_decoded, flash_algo_len);

    // Mem structure: 512 bytes stack + flash algorithm binary + buffer
    ret = swd_write_memory(code_start, (uint8_t *)header_blob, sizeof(header_blob));
    if (ret < 1) {
        ESP_LOGE(TAG, "Failed when writing flash algorithm header");
        state = swd_def::UNKNOWN;
        return ESP_FAIL;
    }

    ret = swd_write_memory(code_start + sizeof(header_blob), flash_algo_decoded, flash_algo_len);
    if (ret < 1) {
        ESP_LOGE(TAG, "Failed when writing main flash algorithm");
        state = swd_def::UNKNOWN;
        return ESP_FAIL;
    }

    ESP_LOGW(TAG, ">>> Readback");
    memset(flash_algo_decoded, 0, 350);
    swd_read_memory(0x20000200, flash_algo_decoded, 350);
    ESP_LOG_BUFFER_HEX(TAG, flash_algo_decoded, 350);

    state = swd_def::FLASH_ALG_LOADED;
    return ESP_OK;
}

esp_err_t swd_prog::run_algo_init(swd_def::init_mode mode)
{
    if (load_flash_algorithm() != ESP_OK) {
        ESP_LOGE(TAG, "Failed when loading flash algorithm");
        return ESP_FAIL;
    }

    auto ret = swd_halt_target();
    if (ret < 1) {
        ESP_LOGE(TAG, "Failed when halting");
        state = swd_def::UNKNOWN;
        return ESP_ERR_INVALID_STATE;
    }

    ret = swd_wait_until_halted();
    if (ret < 1) {
        ESP_LOGE(TAG, "Timeout when halting");
        state = swd_def::UNKNOWN;
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Running init, load_addr: 0x%x, stack_ptr: 0x%x, static_base: 0x%x", syscall.breakpoint, syscall.stack_pointer, syscall.static_base);
    ret = swd_flash_syscall_exec(
            &syscall,
            0x20000221, // Init PC = 1, +0x20 for header
            0x08000000, // r1 = flash base addr
            0, // r2 = ignored
            mode, 0, // r3 = mode, r4 ignored
            FLASHALGO_RETURN_BOOL
    );

    if (ret < 1) {
        ESP_LOGE(TAG, "Failed when init algorithm");
        state = swd_def::UNKNOWN;
        return ESP_FAIL;
    }

    state = swd_def::FLASH_ALG_INITED;
    return ESP_OK;
}

esp_err_t swd_prog::run_algo_uninit(swd_def::init_mode mode)
{
    auto ret = swd_halt_target();
    if (ret < 1) {
        ESP_LOGE(TAG, "Failed when halting");
        state = swd_def::UNKNOWN;
        return ESP_ERR_INVALID_STATE;
    }

    ret = swd_wait_until_halted();
    if (ret < 1) {
        ESP_LOGE(TAG, "Timeout when halting");
        state = swd_def::UNKNOWN;
        return ESP_ERR_INVALID_STATE;
    }

    ret = swd_flash_syscall_exec(
            &syscall,
            code_start + 61, // UnInit PC = 61
            mode,
            0, 0, 0, // r2, r3 = ignored
            FLASHALGO_RETURN_BOOL
    );

    if (ret < 1) {
        ESP_LOGE(TAG, "Failed when uninit algorithm");
        state = swd_def::UNKNOWN;
        return ESP_FAIL;
    }

    state = swd_def::FLASH_ALG_UNINITED;
    return ESP_OK;
}

esp_err_t swd_prog::init()
{
    auto ret = swd_init_debug();
    if (ret < 1) {
        ESP_LOGE(TAG, "Failed when init");
        state = swd_def::UNKNOWN;
        return ESP_FAIL;
    }

    ret = swd_halt_target();
    if (ret < 1) {
        ESP_LOGE(TAG, "Failed when halting");
        state = swd_def::UNKNOWN;
        return ESP_ERR_INVALID_STATE;
    }

    ret = swd_wait_until_halted();
    if (ret < 1) {
        ESP_LOGE(TAG, "Timeout when halting");
        state = swd_def::UNKNOWN;
        return ESP_ERR_INVALID_STATE;
    }

    // TODO: load from JSON
    code_start = 512 + 0x20000000; // Stack size = 512
    syscall.breakpoint = code_start + 1; // This is ARM
    syscall.static_base = 0x11c + code_start + 0x20; // Length = 284, header = 32 bytes
    syscall.stack_pointer = code_start;

    state = swd_def::INITIALISED;
    return ESP_OK;
}

esp_err_t swd_prog::erase_sector(uint32_t start_addr, uint32_t sector_size, uint32_t end_addr)
{
    uint32_t sector_cnt = (end_addr - start_addr) / sector_size;
    if ((end_addr - start_addr) % sector_size != 0 || sector_cnt < 1) {
        ESP_LOGE(TAG, "Misaligned sector address");
        return ESP_ERR_INVALID_ARG;
    }


    if (state != swd_def::FLASH_ALG_INITED) {
        ESP_LOGW(TAG, "Flash alg not initialised, doing now");
        auto ret = run_algo_init(swd_def::ERASE);
        if (ret != ESP_OK) return ret;
    }

    auto swd_ret = swd_halt_target();
    if (swd_ret < 1) {
        ESP_LOGE(TAG, "Failed when halting");
        state = swd_def::UNKNOWN;
        return ESP_ERR_INVALID_STATE;
    }

    swd_ret = swd_wait_until_halted();
    if (swd_ret < 1) {
        ESP_LOGE(TAG, "Timeout when halting");
        state = swd_def::UNKNOWN;
        return ESP_ERR_INVALID_STATE;
    }

    for (uint32_t idx = 0; idx < sector_cnt; idx += 1) {
        swd_ret = swd_flash_syscall_exec(
                &syscall,
                code_start + 91, // ErasePage PC = 91
                0x08000000 + (idx * sector_size), // r1 = flash base addr
                0, 0, 0, // r2, r3 = ignored
                FLASHALGO_RETURN_BOOL
        );

        if (swd_ret < 1) {
            ESP_LOGE(TAG, "Erase function returned an unknown error");
            return ESP_FAIL;
        }
    }

    auto ret = run_algo_uninit(swd_def::ERASE);
    if (ret != ESP_OK) return ret;

    state = swd_def::FLASH_ALG_UNINITED;
    return ret;
}

esp_err_t swd_prog::program_page(uint32_t start_addr, const uint8_t *buf, size_t len)
{
    if (len % 4 != 0) {
        ESP_LOGE(TAG, "Length is not 32-bit word aligned");
        return ESP_ERR_INVALID_ARG;
    }

    if (state != swd_def::FLASH_ALG_INITED) {
        auto ret = run_algo_init(swd_def::PROGRAM);
        if (ret != ESP_OK) return ret;
    }

    auto swd_ret = swd_halt_target();
    if (swd_ret < 1) {
        ESP_LOGE(TAG, "Failed when halting");
        state = swd_def::UNKNOWN;
        return ESP_ERR_INVALID_STATE;
    }


    swd_ret = swd_wait_until_halted();
    if (swd_ret < 1) {
        ESP_LOGE(TAG, "Timeout when halting");
        state = swd_def::UNKNOWN;
        return ESP_ERR_INVALID_STATE;
    }

    swd_ret = swd_write_memory(0x20000a00, (uint8_t *)buf, len); // TODO: wrap or limit length?
    if (swd_ret < 1) {
        ESP_LOGE(TAG, "Failed when writing RAM cache");
        state = swd_def::UNKNOWN;
        return ESP_ERR_INVALID_STATE;
    }

    swd_ret = swd_flash_syscall_exec(
            &syscall,
            code_start + 149, // ErasePage PC = 91
            0x08000080, // r1 = flash base addr
            1024, 0x20000a00, 0, // r2 = len, r3 = buf addr
            FLASHALGO_RETURN_BOOL
    );

    if (swd_ret < 1) {
        ESP_LOGE(TAG, "Program function returned an unknown error");
        state = swd_def::UNKNOWN;
        return ESP_ERR_INVALID_STATE;
    }

    auto ret = run_algo_uninit(swd_def::PROGRAM);
    if (ret != ESP_OK) return ret;

    state = swd_def::FLASH_ALG_UNINITED;
    return ret;
}
