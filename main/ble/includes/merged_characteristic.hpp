#pragma once

#include <cstring>
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

    void onWrite(NimBLECharacteristic* pCharacteristic) override
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
        final_chunk = ((header & 0b01) != 0);
        bool first_chunk = ((header & 0b10) != 0);
        if (!first_chunk && !begin_recv) {
            ESP_LOGW(TAG, "Invalid state - pkt manifest not received before, ignoring");
            return;
        }

        size_t data_offset = 1; // Skip the first byte
        if (first_chunk) {
            expected_len = (data[4] << 24U) | (data[3] << 16U) | (data[2] << 8U) | data[1];
            size_t free_max_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);

            //  Length must be smaller than 3/4 of the max free block of heap
            size_t max_len = (free_max_block * 3) / 4;
            if (max_len < expected_len) {
                ESP_LOGE(TAG, "Heap can only allocate %u bytes while incoming %u bytes", max_len, expected_len);
                return;
            } else {
                ESP_LOGI(TAG, "Heap max block: %u", free_max_block);
            }

            expected_crc = (data[8] << 24U) | (data[7] << 16U) | (data[6] << 8U) | data[5];
            pkt_buf = (uint8_t *) heap_caps_malloc(expected_len, MALLOC_CAP_SPIRAM);
            if (pkt_buf != nullptr) {
                ESP_LOGE(TAG, "Failed to allocate packet buf for len: %u", expected_len);
                return;
            }

            memset(pkt_buf, 0, expected_len);
            begin_recv = true;
            data_offset = 9; // Skip 8 bytes
        }

        if (curr_offset + (len - data_offset) > expected_len) {
            ESP_LOGE(TAG, "Buffer is gonna overflow, resetting the state...");
            cleanup();
            return;
        }

        memcpy(pkt_buf + curr_offset, data + data_offset, len - data_offset);
        curr_offset += len - data_offset;

        if (final_chunk) {
            uint32_t actual_crc = esp_crc32_le(0, pkt_buf, expected_len);
            if (actual_crc != expected_crc) {
                ESP_LOGE(TAG, "Invalid CRC, actual 0x%04x vs expected 0x%04x", actual_crc, expected_crc);
                cleanup();
                return;
            } else {
                on_merged_packet();
                cleanup();
            }
        }
    }

    virtual void on_merged_packet() = 0;

private:
    static const constexpr char *TAG = "merged_char";
    bool begin_recv = false;
    bool final_chunk = false;

protected:
    size_t expected_len = 0;
    size_t curr_offset = 0;
    uint8_t *pkt_buf = nullptr;
    uint32_t expected_crc = 0;

private:
    void cleanup()
    {
        free(pkt_buf);
        expected_crc = 0;
        expected_len = 0;
        final_chunk = false;
        begin_recv = false;
    }
};