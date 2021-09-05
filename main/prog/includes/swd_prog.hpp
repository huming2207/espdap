#pragma once

#include <esp_err.h>
#include <swd_host.h>

namespace swd_def
{
    enum state : uint8_t
    {
        UNKNOWN = 0,
        INITIALISED = 1,
        FLASH_ALG_LOADED = 2,
        FLASH_ALG_INITED = 3,
        FLASH_ALG_RUNNING = 4,
        FLASH_ALG_UNINITED = 5,
    };

    enum init_mode : uint8_t
    {
        ERASE = 1,
        PROGRAM = 2,
        VERIFY = 3,
    };
}


class swd_prog
{
public:
    static swd_prog& instance()
    {
        static swd_prog instance;
        return instance;
    }

    swd_prog(swd_prog const &) = delete;
    void operator=(swd_prog const &) = delete;

private:
    swd_def::state state = swd_def::UNKNOWN;
    program_syscall_t syscall = {};
    uint32_t code_start = 0;

    static constexpr char flash_algo[] = "ASoB0AIqF9E7SIFpDyISAhFDgWE5ScFgOUnBYDlJAWE5SQFhwGnAAgbUOUg3SQFgBiFBYDdJgWAAIHBHA"
                                     "SgB0AIoCNEsSEFoAiIRQ0FgQWgBIhFDQWAAIHBHMLUmSUpoTBUiQ0pgSmgIJSpDSmAAIgJgKUgmSgDgEGC"
                                     "LadsH+9FIaKBDSGBIaKhDSGAAIDC9ASBwR/C1GEwAIyUVCCY/MYkJjEYk4GFoKUNhYGFoMUNhYEAhgMqAwA"
                                     "kfACn60RZJp2n/BwLQEk85YPnnoWkJBQkPBtCgaQ8hCQIIQ6BhASDwvWFoqUNhYGFosUNhYFscnEXY2AAg"
                                     "8L0AIAJA782riQUEAwK/rp2MFhUUE1VVAAAAMABA/w8AAKqqAAAAAAAA";

private:
    swd_prog() = default;
    esp_err_t load_flash_algorithm();
    esp_err_t run_algo_init(swd_def::init_mode mode);
    esp_err_t run_algo_uninit(swd_def::init_mode mode);

public:
    esp_err_t init();
    esp_err_t erase_sector(uint32_t start_addr, uint32_t sector_size, uint32_t end_addr);
    esp_err_t program_page(uint32_t start_addr, const uint8_t *buf, size_t len);


};
