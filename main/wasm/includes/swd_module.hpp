#pragma once

#include "wasm_module.hpp"

class swd_module : public wasm_module
{
public:
    esp_err_t init() override;
    esp_err_t deinit() override;
    const char *name() const override;
    size_t func_count() const override;

private:
    static int32_t swd_halt_target();
    static int32_t swd_wait_until_halt(int32_t timeout_ms);
    static int32_t swd_read_mem_8();
    static int32_t swd_write_mem_8();
    static int32_t swd_read_mem_32();
    static int32_t swd_write_mem_32();
    static int32_t swd_read_idcode(uint32_t *idcode);
    static int32_t swd_erase_flash();
    static int32_t swd_erase_sector();
    static int32_t swd_program_file(char *path, bool verify);
    static int32_t swd_program_buf(uint8_t *buf, size_t len, bool verify);
};

