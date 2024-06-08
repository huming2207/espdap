#pragma once

#include <cstdint>
#include <ArduinoJson.hpp>
#include <psram_json_allocator.hpp>

namespace rpc::report
{
    struct base_event
    {
    protected:
        PsRamAllocator allocator;
        ArduinoJson::JsonDocument document;

    public:
        explicit base_event() : allocator{}, document(&allocator) {}
        virtual size_t serialize(uint8_t *buf_out, size_t buf_size) = 0;
        virtual size_t get_serialized_size() = 0;
    };

    /**
     * Init event after detected a product
     *
     * @remark "algo" - Flash algorithm ELF file hash in SHA256
     * @remark "fw" - Firmware binary hash in SHA256
     * @remark "sn" - Serial number detected from target product
     */
    struct init_event : public base_event
    {
    public:
        size_t get_serialized_size() override
        {
            document.clear();
            document["algo"] = ArduinoJson::MsgPackBinary(flash_algo_hash, sizeof(flash_algo_hash));
            document["fw"] = ArduinoJson::MsgPackBinary(firmware_hash, sizeof(firmware_hash));
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, target_sn_len);

            return ArduinoJson::measureMsgPack(document);
        };

        size_t serialize(uint8_t *buf_out, size_t buf_size) override
        {
            document.clear();
            document["algo"] = ArduinoJson::MsgPackBinary(flash_algo_hash, sizeof(flash_algo_hash));
            document["fw"] = ArduinoJson::MsgPackBinary(firmware_hash, sizeof(firmware_hash));
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, std::min(target_sn_len, sizeof(target_sn)));

            return ArduinoJson::serializeMsgPack(document, (void *)buf_out, buf_size);
        }

    public:
        size_t target_sn_len = 0;
        uint8_t target_sn[32]{};
        uint8_t flash_algo_hash[32]{};
        uint8_t firmware_hash[32]{};
    };


    /**
     * General error event
     *
     * @remark "code" - Error code in esp_err_t, 0 means OK (but should not be reported)
     * @remark "msg" - Message in string
     * @remark "sn" - Serial number detected from target product
     */
    struct error_event : public base_event
    {
    public:
        size_t get_serialized_size() override
        {
            document.clear();
            document["msg"] = msg_str;
            document["code"] = err_code;
            if (target_sn_len != 0) {
                document["sn"] = ArduinoJson::MsgPackBinary(target_sn, target_sn_len);
            }

            return ArduinoJson::measureMsgPack(document);
        };

        size_t serialize(uint8_t *buf_out, size_t buf_size) override
        {
            document.clear();
            document["msg"] = msg_str;
            document["code"] = err_code;
            if (target_sn_len != 0) {
                document["sn"] = ArduinoJson::MsgPackBinary(target_sn, target_sn_len);
            }
            return ArduinoJson::serializeMsgPack(document, (void *)buf_out, buf_size);
        }

    public:
        size_t target_sn_len = 0;
        uint8_t target_sn[32]{};
        esp_err_t err_code = ESP_OK;
        char *msg_str = nullptr;
    };

    /**
     * Program event after triggered by CMD_SET_STATE
     *
     * @remark "algo" - Flash algorithm ELF file hash in SHA256
     * @remark "fw" - Firmware binary hash in SHA256
     * @remark "sn" - Serial number detected from target product
     * @remark "addr" - Beginning address that programmed
     * @remark "len" - Length of the data programmed
     */
    struct prog_event : public base_event
    {
    public:
        size_t get_serialized_size() override
        {
            document.clear();
            document["algo"] = ArduinoJson::MsgPackBinary(flash_algo_hash, sizeof(flash_algo_hash));
            document["fw"] = ArduinoJson::MsgPackBinary(firmware_hash, sizeof(firmware_hash));
            document["addr"] = addr;
            document["len"] = len;
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, target_sn_len);

            return ArduinoJson::measureMsgPack(document);
        };

        size_t serialize(uint8_t *buf_out, size_t buf_size) override
        {
            document.clear();
            document["algo"] = ArduinoJson::MsgPackBinary(flash_algo_hash, sizeof(flash_algo_hash));
            document["fw"] = ArduinoJson::MsgPackBinary(firmware_hash, sizeof(firmware_hash));
            document["addr"] = addr;
            document["len"] = len;
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, std::min(target_sn_len, sizeof(target_sn)));

            return ArduinoJson::serializeMsgPack(document, (void *)buf_out, buf_size);
        }

        uint32_t addr = 0;
        uint32_t len = 0;
        size_t target_sn_len = 0;
        uint8_t target_sn[32]{};
        uint8_t flash_algo_hash[32]{};
        uint8_t firmware_hash[32]{};
    };

    struct self_test_event : public base_event
    {
        size_t get_serialized_size() override
        {
            document.clear();
            document["testID"] = test_id;
            document["ret"] = return_num;
            document["algo"] = ArduinoJson::MsgPackBinary(flash_algo_hash, sizeof(flash_algo_hash));
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, std::min(target_sn_len, sizeof(target_sn)));
            if (ret_buf != nullptr && ret_len == 0) {
                document["retPld"] = ArduinoJson::MsgPackBinary(ret_buf, ret_len);
            }

            return ArduinoJson::measureMsgPack(document);
        }

        size_t serialize(uint8_t *buf_out, size_t buf_size) override
        {
            document.clear();
            document["testID"] = test_id;
            document["ret"] = return_num;
            document["algo"] = ArduinoJson::MsgPackBinary(flash_algo_hash, sizeof(flash_algo_hash));
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, target_sn_len);
            if (ret_buf != nullptr && ret_len == 0) {
                document["retPld"] = ArduinoJson::MsgPackBinary(ret_buf, ret_len);
            }

            return ArduinoJson::serializeMsgPack(document, (void *)buf_out, buf_size);
        }

        uint32_t test_id{};
        uint32_t return_num{};
        uint8_t flash_algo_hash[32]{};
        uint8_t target_sn[32]{};
        size_t target_sn_len = 0;
        uint8_t *ret_buf = nullptr;
        size_t ret_len = 0;
    };

    struct erase_event : public base_event
    {
        size_t get_serialized_size() override
        {
            document.clear();
            document["addr"] = addr;
            document["len"] = len;
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, target_sn_len);

            return ArduinoJson::measureMsgPack(document);
        }

        size_t serialize(uint8_t *buf_out, size_t buf_size) override
        {
            document.clear();
            document["addr"] = addr;
            document["len"] = len;
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, target_sn_len);

            return ArduinoJson::serializeMsgPack(document, (void *)buf_out, buf_size);
        }

        uint32_t addr{};
        uint32_t len{};
        uint8_t target_sn[32]{};
        uint8_t target_sn_len = 0;
    };

    struct repair_event : public base_event
    {
        size_t get_serialized_size() override
        {
            document.clear();
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, target_sn_len);
            if (comment != nullptr && comment_len == 0) {
                document["comment"] = ArduinoJson::MsgPackBinary(comment, comment_len);
            }

            return ArduinoJson::measureMsgPack(document);
        }

        size_t serialize(uint8_t *buf_out, size_t buf_size) override
        {
            document.clear();
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, target_sn_len);
            if (comment != nullptr && comment_len == 0) {
                document["comment"] = ArduinoJson::MsgPackBinary(comment, comment_len);
            }

            return ArduinoJson::serializeMsgPack(document, (void *)buf_out, buf_size);
        }

        uint8_t target_sn[32]{};
        uint8_t target_sn_len = 0;
        char *comment = nullptr;
        size_t comment_len = 0;
    };

    struct dispose_event : public base_event
    {
        size_t get_serialized_size() override
        {
            document.clear();
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, target_sn_len);
            if (comment != nullptr && comment_len == 0) {
                document["comment"] = ArduinoJson::MsgPackBinary(comment, comment_len);
            }

            return ArduinoJson::measureMsgPack(document);
        }

        size_t serialize(uint8_t *buf_out, size_t buf_size) override
        {
            document.clear();
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, target_sn_len);
            if (comment != nullptr && comment_len == 0) {
                document["comment"] = ArduinoJson::MsgPackBinary(comment, comment_len);
            }

            return ArduinoJson::serializeMsgPack(document, (void *)buf_out, buf_size);
        }

        uint8_t target_sn[32]{};
        uint8_t target_sn_len = 0;
        char *comment = nullptr;
        size_t comment_len = 0;
    };
}
