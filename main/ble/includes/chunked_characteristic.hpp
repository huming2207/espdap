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
 *  - When First Chunk = 1:
 *      - Byte 1-4: length
 *      - Byte 5-8: CRC32 of the whole packet
 *  - When First Chunk = 0:
 *      - Byte 1-4: packet length offset
 */
class chunked_characteristic : public NimBLECharacteristicCallbacks
{
public:
    chunked_characteristic() = default;

    void onWrite(NimBLECharacteristic* pCharacteristic) override
    {
        auto data = pCharacteristic->getValue();
        size_t len = pCharacteristic->getValue().size();

        if (len < 5) {
            ESP_LOGE(TAG, "Length of chunk is too short: %u", len);
            generate_reply(pCharacteristic, nullptr, 0, false, true);
            return;
        }

        uint8_t header = data[0];
        final_chunk = ((header & 0b01) != 0);
        bool first_chunk = ((header & 0b10) != 0);
        if (!first_chunk && !begin_recv) {
            ESP_LOGW(TAG, "Invalid state - pkt manifest not received before, ignoring");
            generate_reply(pCharacteristic, nullptr, 0, false, true);
            return;
        }

        size_t header_offset = 1; // The bytes to skip
        uint32_t pkt_offset = 0;
        if (first_chunk) {
            expected_len = (data[4] << 24U) | (data[3] << 16U) | (data[2] << 8U) | data[1];
            size_t free_max_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);

            //  Length must be smaller than 3/4 of the max free block of heap
            size_t max_len = (free_max_block * 3) / 4;
            if (max_len < expected_len) {
                ESP_LOGE(TAG, "Heap can only allocate %u bytes while incoming %u bytes", max_len, expected_len);
                generate_reply(pCharacteristic, nullptr, 0, false, true);
                return;
            } else {
                ESP_LOGI(TAG, "Heap max block: %u", free_max_block);
            }

            expected_crc = (data[8] << 24U) | (data[7] << 16U) | (data[6] << 8U) | data[5];
            pkt_buf = (uint8_t *) heap_caps_malloc(expected_len, MALLOC_CAP_SPIRAM);
            if (pkt_buf == nullptr) {
                ESP_LOGE(TAG, "Failed to allocate packet buf for len: %u", expected_len);
                generate_reply(pCharacteristic, nullptr, 0, false, false, true);
                cleanup();
                return;
            } else {
                memset(pkt_buf, 0, expected_len);
                begin_recv = true;
                header_offset = 9; // Skip 9 bytes
            }
        } else {
            pkt_offset = (data[4] << 24U) | (data[3] << 16U) | (data[2] << 8U) | data[1];
            header_offset = 5;
        }

        memcpy(pkt_buf + pkt_offset, &data[0] + header_offset, len - header_offset);
        generate_reply(pCharacteristic, &data[0], len);

        if (final_chunk) {
            uint32_t actual_crc = esp_crc32_le(0, pkt_buf, expected_len);
            if (actual_crc != expected_crc) {
                ESP_LOGE(TAG, "Invalid CRC, actual 0x%04x vs expected 0x%04x", actual_crc, expected_crc);
                generate_reply(pCharacteristic, nullptr, 0, false, true);
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

    static void generate_reply(
            NimBLECharacteristic *characteristic,
            const uint8_t *buf, size_t len,
            bool retry_chunk = false, bool retry_pkt = false, bool device_side_err = false)
    {
        uint16_t chunk_crc = (buf == nullptr || len < 1) ? 0 : ~esp_crc16_le((uint16_t)~0x0000, buf, len); // CRC-16 CCITT
        uint8_t state_byte =  (device_side_err ? 0b100 : 0) | (retry_pkt ? 0b10 : 0b00) | (retry_chunk ? 0b01 : 0b00);
        uint32_t notify_buf = ((state_byte & 0xff) << 24) | chunk_crc;
        characteristic->setValue(notify_buf);
        characteristic->notify();
    }
};