#include <wasm_export.h>
#include <swd_host.h>
#include <cerrno>
#include "swd_module.hpp"

esp_err_t swd_module::init()
{
    bool ret = wasm_runtime_register_natives(name(), const_cast<NativeSymbol *>(symbols), func_count());
    return ret ? ESP_OK : ESP_FAIL;
}

esp_err_t swd_module::deinit()
{
    // No-op??
    return ESP_OK;
}

const char *swd_module::name() const
{
    return "swd";
}

size_t swd_module::func_count() const
{
    return sizeof(symbols) / sizeof(NativeSymbol);
}

int32_t swd_module::halt_target()
{
    if (swd_halt_target() != 0) {
        return RET_OK;
    } else {
        return EBUSY;
    }
}

int32_t swd_module::wait_till_halt()
{
    if (swd_wait_until_halted() != 0) {
        return RET_OK;
    } else {
        return EBUSY;
    }
}

int32_t swd_module::read_mem_8(uint32_t addr, uint8_t *data_byte)
{
    if (swd_read_byte(addr, data_byte) != 0) {
        return RET_OK;
    }

    return ETIMEDOUT;
}

int32_t swd_module::write_mem_8(uint32_t addr, uint8_t data_byte)
{
    if (swd_write_byte(addr, data_byte) != 0) {
        return RET_OK;
    }

    return ETIMEDOUT;
}

int32_t swd_module::read_mem_32(uint32_t addr, uint32_t *data_word)
{
    if (swd_read_word(addr, data_word) != 0) {
        return RET_OK;
    }

    return ETIMEDOUT;
}

int32_t swd_module::write_mem_32(uint32_t addr, uint32_t data_word)
{
    if (swd_write_word(addr, data_word) != 0) {
        return RET_OK;
    }

    return ETIMEDOUT;
}

uint32_t swd_module::read_idcode()
{
    uint32_t idcode = 0;
    if (swd_read_idcode(&idcode) != 0) {
        return idcode;
    }

    return 0;
}

int32_t swd_module::read_mem_buf(uint32_t addr, uint8_t *buf, uint32_t length)
{
    if (swd_read_memory(addr, buf, length) != 0) {
        return RET_OK;
    } else {
        return EBUSY;
    }
}

int32_t swd_module::write_mem_buf(uint32_t addr, uint8_t *buf, uint32_t length)
{
    if (swd_write_memory(addr, buf, length) != 0) {
        return RET_OK;
    } else {
        return EBUSY;
    }
}
