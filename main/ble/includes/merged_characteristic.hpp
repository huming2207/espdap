#pragma once

#include <cstdio>
#include <cstddef>
#include <esp_heap_caps.h>
#include <esp_crc.h>
#include <esp_err.h>
#include <esp_log.h>

#include <NimBLECharacteristic.h>

/**
 * Packet structure:
 *  - Byte 0:
 *      - Bit 0: 1 => Final chunk; 0 => Not final chunk (wait for more to come)
 *      - Bit 1: 1 => First chunk; 0 => Not first chunk
 *      - Bit 2-7 reserved
 *  - Byte 1-4: length (for the whole packet not just chunks)
 *  - Byte 5-8: CRC32 (for the whole packet)
 */
class merged_characteristic : public NimBLECharacteristicCallbacks
{
public:
    merged_characteristic() = default;

    void onRead(NimBLECharacteristic* pCharacteristic) override
    {
        if (file == nullptr) {
            ESP_LOGE(TAG, "Temp file not open");
            return;
        }

        char *data = pCharacteristic->getValue().data();
        size_t len = pCharacteristic->getValue().length();

        if (len < 5) {
            ESP_LOGE(TAG, "Length of chunk is too short: %u", len);
            return;
        }

        uint8_t header = data[0];
        final_state = ((header & 0b01) != 0);
        bool first_chunk = ((header & 0b10) != 0);
        if (!first_chunk && !begin_recv) {
            ESP_LOGW(TAG, "Invalid state - pkt manifest not received before, ignoring");
            return;
        }

        if (first_chunk) {
            expected_len = (data[4] << 24U) | (data[3] << 16U) | (data[2] << 8U) | data[1];

            //  Length must be smaller than 3/4 of the max free block of heap
            size_t max_len = (heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT) * 3) / 4;
            if (max_len < expected_len) {
                ESP_LOGE(TAG, "Heap can only allocate %u bytes while incoming %u bytes", max_len, expected_len);
                return;
            }

            expected_crc = (data[8] << 24U) | (data[7] << 16U) | (data[6] << 8U) | data[5];
            begin_recv = true;
        } else {

        }
    }

    virtual void on_merged_packet() = 0;

private:
    static const constexpr char *TAG = "merged_char";
    bool begin_recv = false;
    bool final_state = false;
    size_t expected_len = 0;
    size_t curr_offset = 0;
    uint8_t *pkt_buf = nullptr;
    uint32_t expected_crc = 0;
};