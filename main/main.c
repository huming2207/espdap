#include <stdio.h>
#include <string.h>
#include <swd_host.h>
#include <esp_log.h>
#include <esp_timer.h>

void app_main(void)
{
    static const char *TAG = "main";
    ESP_LOGI(TAG, "Lay hou from DAPLink!");

    swd_init_debug();

    uint32_t idcode = 0;
    uint8_t ret = swd_read_idcode(&idcode);

    ESP_LOGI(TAG, "IDCode readout is: 0x%x, ret: %u", idcode, ret);

    uint8_t *buf = malloc(10240);
    memset(buf, 0x5a, 10240);

    int64_t start_ts = esp_timer_get_time();
    swd_write_memory(0x20000000, buf, 10240);
    ESP_LOGI(TAG, "Wrote 10KB used %lld us", esp_timer_get_time() - start_ts);
}
