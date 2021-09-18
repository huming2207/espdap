#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <swd_host.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <cstring>
#include <mbedtls/base64.h>
#include "swd_prog.hpp"

#define TAG "swd_prog"

// From: https://github.com/probe-rs/probe-rs/blob/master/probe-rs/targets/STM32L0_Series.yaml#L3997
const static constexpr char flash_algo[] = "gLWEsACv+GC5YHpgE0uaaRJL8CEJAQpDmmEQSxBK2mAOSxBK2mANSw9KGmELSw9KGmEKS9ppgCNbAxNACNEMSwxKGmAKSwYiWmAJSwpKmmAAIxgAvUYEsIC9wEYAIAJA782riQUEAwK/rp2MFhUUEwAwAEBVVQAA/w8AAIC1grAAr3hgCEtaaAdLAiEKQ1pgBUtaaARLASEKQ1pgACMYAL1GArCAvcBGACACQIC1grAAr3hgFEtaaBNLgCGJAApDWmARS1poEEsIIQpDWmB7aAAiGmAC4A1LDUoaYApLm2kBIhNA99EIS1poB0sJSQpAWmAFS1poBEsIIYpDWmAAIxgAvUYCsIC9ACACQAAwAECqqgAA//3//4C1hLAAr/hguWD7HRpwASMYAL1GBLCAvYC1hrAAr/hguWB6YLtoPzM/IpNDu2AAIzthRuAoS1poJ0uAIckACkNaYCVLWmgkSwghCkNaYEAje2EM4PtoemgSaBpg+2gEM/tge2gEM3tge2kEO3the2kAK+/RAuAZSxlKGmAWS5tpASITQPfRFEuaafAjGwETQAjQEUuaaRBL8CEJAQpDmmEBIxTgDEtaaAtLDkkKQFpgCUtaaAhLCCGKQ1pgO2kBMzthu2ibCTppmkKz0wAjGAC9RgawgL3ARgAgAkAAMABAqqoAAP/7//8=";

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

    uint8_t flash_algo_decoded[768] = { 0 };
    size_t flash_algo_len = 0;
    int mbedtls_ret = mbedtls_base64_decode(
                flash_algo_decoded, sizeof(flash_algo_decoded),
                &flash_algo_len,
                (uint8_t *)flash_algo, strlen(flash_algo)
            );

    ESP_LOGI(TAG, "Mbedtls returned %d, flash algo len %u, loading to 0x%x", mbedtls_ret, flash_algo_len, (uint32_t)(code_start + sizeof(header_blob)));
//    ESP_LOG_BUFFER_HEX(TAG, header_blob, sizeof(header_blob));
//    ESP_LOG_BUFFER_HEX(TAG, flash_algo_decoded, flash_algo_len);

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

    state = swd_def::FLASH_ALG_LOADED;
    return ESP_OK;
}

esp_err_t swd_prog::run_algo_init(swd_def::init_mode mode)
{
    ESP_LOGI(TAG, "Running init, load_addr: 0x%x, stack_ptr: 0x%x, static_base: 0x%x", syscall.breakpoint, syscall.stack_pointer, syscall.static_base);
    uint32_t retry_cnt = 3;
    while (retry_cnt > 0) {
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

        ret = swd_flash_syscall_exec(
                &syscall,
                0x20000221, // Init PC = 1, +0x20 for header (but somehow actually 0?)
                0x08000000, // r0 = flash base addr
                0, // r1 = ignored
                mode, 0, // r2 = mode, r3 ignored
                FLASHALGO_RETURN_BOOL
        );

        if (ret < 1) {
            ESP_LOGW(TAG, "Failed when init algorith, retrying...");
            init(); // Re-init SWD as well (so that target will reset)
            retry_cnt -= 1;
        } else {
            state = swd_def::FLASH_ALG_INITED;
            return ESP_OK;
        }
    }

    state = swd_def::UNKNOWN;
    return ESP_FAIL;
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
            code_start + 125 + 0x20, // UnInit PC = 61
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
    syscall.static_base = 512 + code_start + 0x20; // Length = 512, header = 32 bytes
    syscall.stack_pointer = code_start;

    state = swd_def::INITIALISED;
    return ESP_OK;
}

esp_err_t swd_prog::erase_sector(uint32_t start_addr, uint32_t sector_size, uint32_t end_addr)
{
    uint32_t sector_cnt = (end_addr - start_addr) / sector_size;
    ESP_LOGI(TAG, "End addr 0x%x, start addr 0x%x, sector size %u", end_addr, start_addr, sector_size);
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

    for (uint32_t idx = 0; idx < sector_cnt - 1; idx += 1) {
        swd_ret = swd_flash_syscall_exec(
                &syscall,
                code_start + 0x20 + 173, // ErasePage PC = 173
                0x08000000 + (idx * sector_size), // r0 = flash base addr
                0, 0, 0, // r1, r2 = ignored
                FLASHALGO_RETURN_BOOL
        );

//        ESP_LOGW(TAG, "Erased at 0x%x, idx: %u of %u", 0x08000000 + (idx * sector_size), idx, sector_cnt -1);

        if (swd_ret < 1) {
            ESP_LOGE(TAG, "Erase function returned an unknown error");
            return ESP_FAIL;
        }

        vTaskDelay(1);
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

    swd_ret = swd_write_memory(0x20000f00, (uint8_t *)buf, len); // TODO: wrap or limit length?
    if (swd_ret < 1) {
        ESP_LOGE(TAG, "Failed when writing RAM cache");
        state = swd_def::UNKNOWN;
        return ESP_ERR_INVALID_STATE;
    }

    for (uint32_t page_idx = 0; page_idx < len / 1024; page_idx += 1) {
        ESP_LOGI(TAG, "Writing page 0x%x", 0x08000000 + (page_idx * 1024));
        swd_ret = swd_flash_syscall_exec(
                &syscall,
                code_start + 305 + 0x20, // ErasePage PC = 305
                0x08000000 + (page_idx * 1024), // r0 = flash base addr
                1024,
                0x20000f00, 0, // r1 = len, r2 = buf addr
                FLASHALGO_RETURN_BOOL
        );

        vTaskDelay(1);
    }

    if (swd_ret < 1) {
        ESP_LOGE(TAG, "Program function returned an unknown error");
        uint32_t err_sr = 0;
        swd_read_word(0x40022018, &err_sr);
        ESP_LOGW(TAG, "FLASH_SR = 0x%x", err_sr);

        state = swd_def::UNKNOWN;
        return ESP_ERR_INVALID_STATE;
    }

    auto ret = run_algo_uninit(swd_def::PROGRAM);
    if (ret != ESP_OK) return ret;

    state = swd_def::FLASH_ALG_UNINITED;
    return ret;
}
