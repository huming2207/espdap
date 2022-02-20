#pragma once

#include <wasm_export.h>

#include "wasm_module.hpp"

class swd_module : public wasm_module
{
public:
    esp_err_t init() override;
    esp_err_t deinit() override;
    const char *name() const override;
    size_t func_count() const override;

private:
    static int32_t halt_target();
    static int32_t wait_till_halt();
    static int32_t read_mem_8(uint32_t addr, uint8_t *data_byte);
    static int32_t write_mem_8(uint32_t addr, uint8_t data_byte);
    static int32_t read_mem_32(uint32_t addr, uint32_t *data_word);
    static int32_t write_mem_32(uint32_t addr, uint32_t data_word);
    static int32_t read_mem_buf(uint32_t addr, uint8_t *buf, uint32_t length);
    static int32_t write_mem_buf(uint32_t addr, uint8_t *buf, uint32_t length);
    static uint32_t read_idcode();

    constexpr const static NativeSymbol symbols[] = {
            { "halt_target", (void *)halt_target, "()i", nullptr },
            { "wait_till_halt", (void *)wait_till_halt, "()i", nullptr },
            { "read_mem_8", (void *)read_mem_8, "(i*)i", nullptr },
            { "write_mem_8", (void *)write_mem_8, "(ii)i", nullptr },
            { "read_mem_32", (void *)read_mem_32, "(i*)i", nullptr },
            { "write_mem_32", (void *)write_mem_32, "(ii)i", nullptr },
            { "read_mem_buf", (void *)read_mem_buf, "(i*~)i", nullptr },
            { "write_mem_buf", (void *)write_mem_buf, "(i*~)i", nullptr },
            { "read_idcode", (void *)halt_target, "()i", nullptr },
    };
};

