#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <swd_host.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <cstring>
#include "swd_prog.hpp"

#define TAG "swd_prog"

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

    // Mem structure: 512 bytes stack + flash algorithm binary + buffer
    ret = swd_write_memory(code_start, (uint8_t *)header_blob, sizeof(header_blob));
    if (ret < 1) {
        ESP_LOGE(TAG, "Failed when writing flash algorithm header");
        state = swd_def::UNKNOWN;
        return ESP_FAIL;
    }

    ret = swd_write_memory(code_start + sizeof(header_blob), algo->get_algo_bin(), algo->get_algo_bin_len());
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

        auto pc_init = algo->get_pc_init();

        ret = swd_wait_until_halted();
        if (ret < 1) {
            ESP_LOGE(TAG, "Timeout when halting");
            state = swd_def::UNKNOWN;
            return ESP_ERR_INVALID_STATE;
        }

        ret = swd_flash_syscall_exec(
                &syscall,
                func_offset + pc_init, // Init PC (usually) = 1, +0x20 for header (but somehow actually 0?)
                algo->get_flash_start_addr(), // r0 = flash base addr
                0, // r1 = ignored
                mode, 0, // r2 = mode, r3 ignored
                FLASHALGO_RETURN_BOOL
        );

        if (ret < 1) {
            ESP_LOGW(TAG, "Failed when init algorith, retrying...");
            init(algo, ram_addr, stack_size); // Re-init SWD as well (so that target will reset)
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

    auto pc_init = algo->get_pc_uninit();

    ret = swd_wait_until_halted();
    if (ret < 1) {
        ESP_LOGE(TAG, "Timeout when halting");
        state = swd_def::UNKNOWN;
        return ESP_ERR_INVALID_STATE;
    }

    ret = swd_flash_syscall_exec(
            &syscall,
            func_offset + pc_init, // UnInit PC = 61
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

esp_err_t swd_prog::init(flash_algo *_algo, uint32_t _ram_addr, uint32_t _stack_size_byte)
{
    if (_algo == nullptr) {
        ESP_LOGE(TAG, "Flash algorithm container pointer is null");
        return ESP_ERR_INVALID_ARG;
    }

    if (_stack_size_byte == 0) {
        ESP_LOGE(TAG, "Stack size too small");
        return ESP_ERR_INVALID_ARG;
    }

    algo = _algo;
    ram_addr = _ram_addr;
    stack_size = _stack_size_byte;

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
    code_start = ram_addr + stack_size; // Stack size (default) = 512 -> offset = 0x20000200
    func_offset = code_start + sizeof(header_blob);
    syscall.breakpoint = code_start + 1; // This is ARM
    syscall.static_base = 512 + code_start + 0x20; // Length = 512, header = 32 bytes
    syscall.stack_pointer = code_start;

    state = swd_def::INITIALISED;
    return ESP_OK;
}

esp_err_t swd_prog::erase_sector(uint32_t start_addr, uint32_t end_addr)
{
    auto flash_sector_size = algo->get_sector_size();
    uint32_t sector_cnt = (end_addr - start_addr) / flash_sector_size;
    ESP_LOGI(TAG, "End addr 0x%x, start addr 0x%x, sector size %u", end_addr, start_addr, flash_sector_size);
    if ((end_addr - start_addr) % flash_sector_size != 0 || sector_cnt < 1) {
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

    auto pc_erase_sector = algo->get_pc_erase_sector();
    auto flash_start_addr = algo->get_flash_start_addr();

    swd_ret = swd_wait_until_halted();
    if (swd_ret < 1) {
        ESP_LOGE(TAG, "Timeout when halting");
        state = swd_def::UNKNOWN;
        return ESP_ERR_INVALID_STATE;
    }

    for (uint32_t idx = 0; idx < sector_cnt - 1; idx += 1) {
        swd_ret = swd_flash_syscall_exec(
                &syscall,
                func_offset + pc_erase_sector, // ErasePage PC = 173
                flash_start_addr + (idx * flash_sector_size), // r0 = flash base addr
                0, 0, 0, // r1, r2 = ignored
                FLASHALGO_RETURN_BOOL
        );

        if (swd_ret < 1) {
            ESP_LOGE(TAG, "Erase function returned an unknown error");
            return ESP_FAIL;
        }

        if(idx % 10 == 0) {
            led.set_color(0, 0, 160, 50);
        } else {
            led.set_color(0, 0, 0, 50);
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

    auto pc_program_page = algo->get_pc_program_page();
    auto flash_start_addr = algo->get_flash_start_addr();
    auto page_size = algo->get_page_size();

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
                func_offset + pc_program_page, // ErasePage PC = 305
                flash_start_addr + (page_idx * page_size), // r0 = flash base addr
                page_size,
                0x20000f00, 0, // r1 = len, r2 = buf addr
                FLASHALGO_RETURN_BOOL
        );

        if(page_idx % 2 == 0) {
            led.set_color(60, 60, 0, 20);
        } else {
            led.set_color(0, 0, 0, 20);
        }
    }

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
