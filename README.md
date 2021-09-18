# Soul Injector

Offline firmware downloader for ARM Cortex-M, on an ESP32(-S2/3).

Tested on ESP32-S2 only but it should work on ESP32-S3.

## Usage

target is a STM32L031K6, writing 8KB of 0x5a to RAM:

```c
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

    uint8_t *buf = malloc(8192);
    memset(buf, 0x5a, 8192);

    int64_t start_ts = esp_timer_get_time();
    ret = swd_write_memory(0x20000000, buf, 8192);
    ESP_LOGI(TAG, "Wrote 8KB used %lld us, ret %u", esp_timer_get_time() - start_ts, ret);
}

```

Log output:

```
I (495) main: Lay hou from DAPLink!
I (505) main: IDCode readout is: 0xbc11477, ret: 1
I (535) main: Wrote 8KB used 24842 us, ret 1
```

## To-do list

- [x] Run flash algorithm
- [ ] Offline firmware flashing
- [ ] More tests
- [ ] Read verification
- [ ] ESP32-to-ESP32 firmware downloading
 
## Post-MVP feature list

- [ ] USB DAP-Link probe? (but why not buy one from Taobao instead?)
- [ ] OCD server over BLE/WiFi??

## License

- This project: 
  - Dual license:
    - GPLv3 for non-commercial purposes
    - Commercial licenses for any commercial purposes, speak to me (the author) for more details.
  - DAPLink port (under `components/daplink-esp`):
    - MIT
