#include <stdio.h>
#include <string.h>
#include <swd_host.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <mbedtls/base64.h>

// From: https://github.com/probe-rs/probe-rs/blob/master/probe-rs/targets/STM32L0_Series.yaml#L3997
static const char flash_algo[] = "ASoB0AIqF9E7SIFpDyISAhFDgWE5ScFgOUnBYDlJAWE5SQFhwGnAAgbUOUg3SQFgBiFBYDdJgWAAIHBHA"
                                "SgB0AIoCNEsSEFoAiIRQ0FgQWgBIhFDQWAAIHBHMLUmSUpoTBUiQ0pgSmgIJSpDSmAAIgJgKUgmSgDgEGC"
                                "LadsH+9FIaKBDSGBIaKhDSGAAIDC9ASBwR/C1GEwAIyUVCCY/MYkJjEYk4GFoKUNhYGFoMUNhYEAhgMqAwA"
                                "kfACn60RZJp2n/BwLQEk85YPnnoWkJBQkPBtCgaQ8hCQIIQ6BhASDwvWFoqUNhYGFosUNhYFscnEXY2AAg"
                                "8L0AIAJA782riQUEAwK/rp2MFhUUE1VVAAAAMABA/w8AAKqqAAAAAAAA";


void app_main(void)
{
    static const char *TAG = "main";
    ESP_LOGI(TAG, "Lay hou from DAPLink!");

    swd_init_debug();
    swd_halt_target();
    uint8_t halt_ret = swd_wait_until_halted();
    ESP_LOGI(TAG, "Wait halt: %u", halt_ret);

    uint32_t idcode = 0;
    uint8_t ret = swd_read_idcode(&idcode);

    ESP_LOGI(TAG, "IDCode readout is: 0x%x, ret: %u", idcode, ret);

    ret = swd_halt_target();
    ret = (ret == 0) ? ret : swd_wait_until_halted();
    ESP_LOGI(TAG, "Halt result: %d", ret);

//    uint8_t *buf = malloc(20480);
//    memset(buf, 0x00, 20480);

//    int64_t start_ts = esp_timer_get_time();
//    ret = swd_write_memory(0x20000000, buf, 20480);
//    ESP_LOGI(TAG, "Wrote 20KB used %lld us, ret %u", esp_timer_get_time() - start_ts, ret);

    uint8_t flash_algo_decoded[300] = { 0 };
    size_t flash_algo_len = 0;
    int mbedtls_ret = mbedtls_base64_decode(flash_algo_decoded, sizeof(flash_algo_decoded), &flash_algo_len, (uint8_t *)flash_algo, strlen(flash_algo));

    ESP_LOGI(TAG, "Mbedtls returned %d, flash algo len %u", mbedtls_ret, flash_algo_len);

    // Mem structure: 512 bytes stack + flash algorithm binary + buffer
    uint32_t code_start = 512 + 0x20000000; // Stack size = 512

    int64_t start_ts = esp_timer_get_time();
    ret = swd_write_memory(code_start, flash_algo_decoded, flash_algo_len);
    ESP_LOGI(TAG, "Wrote flash algorithm used %lld us, ret %u", esp_timer_get_time() - start_ts, ret);

    swd_halt_target();
    halt_ret = swd_wait_until_halted();
    ESP_LOGI(TAG, "Wait halt: %u", halt_ret);

    program_syscall_t syscall = {
            .breakpoint = 0x20000001,
            .static_base = 0x11c + code_start,
            .stack_pointer = 0x20000800,
    };

    ret = swd_flash_syscall_exec(
            &syscall,
            code_start + 0x20 + 1, // Init PC = 1
            0x08000000, // r1 = flash base addr
            0, 1, 0, // r2, r3 = ignored
            FLASHALGO_RETURN_BOOL
    );

    swd_halt_target();
    halt_ret = swd_wait_until_halted();
    ESP_LOGI(TAG, "Wait halt: %u", halt_ret);

    ESP_LOGI(TAG, "INIT result: %u", ret);

    for (int sector_idx = 0; sector_idx < 1024; sector_idx++) {
        ret = swd_flash_syscall_exec(
                &syscall,
                code_start + 91, // ErasePage PC = 91
                0x08000000 + (sector_idx * 128), // r1 = flash base addr
                0, 0, 0, // r2, r3 = ignored
                FLASHALGO_RETURN_BOOL
        );

    }


    ESP_LOGI(TAG, "Erase sector done");
}
