#pragma once

#include <cstdint>
#include <ArduinoJson.hpp>
#include <psram_json_allocator.hpp>

namespace rpc
{
    enum event_type : uint8_t
    {
        EVENT_PROGRAM = 0x10,
        EVENT_TEST_INTERNAL = 0x20,
        EVENT_TEST_EXTERNAL = 0x21, // Do we need this?
        EVENT_ERASE = 0x30,
        EVENT_REPAIR = 0x40,
        EVENT_DISPOSE = 0x50,
    };

    struct base_event
    {
    protected:
        PsRamAllocator allocator;
        ArduinoJson::JsonDocument document;

    public:
        event_type type;
        explicit base_event(event_type _type) : allocator{}, document(&allocator), type(_type) {}
        virtual size_t serialize(uint8_t *buf_out, size_t buf_size) = 0;
        virtual size_t get_serialized_size() = 0;
    };

    struct prog_event : public base_event
    {
        prog_event() : base_event(EVENT_PROGRAM) {}

        size_t get_serialized_size() override
        {
            document.clear();
            document["type"] = type;
            document["projID"] = project_id;
            document["addr"] = addr;
            document["len"] = len;
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, target_sn_len);

            return ArduinoJson::measureMsgPack(document);
        };

        size_t serialize(uint8_t *buf_out, size_t buf_size) override
        {
            document.clear();
            document["type"] = type;
            document["projID"] = project_id;
            document["addr"] = addr;
            document["len"] = len;
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, target_sn_len);

            return ArduinoJson::serializeMsgPack(document, (void *)buf_out, buf_size);
        }

        uint32_t project_id{};
        uint32_t addr{};
        uint32_t len{};
        uint8_t target_sn[32]{};
        uint8_t target_sn_len = 0;
    };

    struct self_test_event : public base_event {
        self_test_event() : base_event(EVENT_TEST_INTERNAL) {}

        size_t get_serialized_size() override
        {
            document.clear();
            document["type"] = type;
            document["testID"] = test_id;
            document["ret"] = return_num;
            document["name"] = test_name;
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, target_sn_len);
            if (ret_buf != nullptr && ret_len == 0) {
                document["retPld"] = ArduinoJson::MsgPackBinary(ret_buf, ret_len);
            }

            return ArduinoJson::measureMsgPack(document);
        }

        size_t serialize(uint8_t *buf_out, size_t buf_size) override
        {
            document.clear();
            document["type"] = type;
            document["testID"] = test_id;
            document["ret"] = return_num;
            document["name"] = test_name;
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, target_sn_len);
            if (ret_buf != nullptr && ret_len == 0) {
                document["retPld"] = ArduinoJson::MsgPackBinary(ret_buf, ret_len);
            }

            return ArduinoJson::serializeMsgPack(document, (void *)buf_out, buf_size);
        }

        uint32_t test_id{};
        uint32_t return_num{};
        char test_name[32]{};
        uint8_t target_sn[32]{};
        uint8_t target_sn_len = 0;
        uint8_t *ret_buf = nullptr;
        size_t ret_len = 0;
    };

    struct erase_event : public base_event
    {
        erase_event() : base_event(EVENT_ERASE) {}

        size_t get_serialized_size() override
        {
            document.clear();
            document["type"] = type;
            document["addr"] = addr;
            document["len"] = len;
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, target_sn_len);

            return ArduinoJson::measureMsgPack(document);
        }

        size_t serialize(uint8_t *buf_out, size_t buf_size) override
        {
            document.clear();
            document["type"] = type;
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
        repair_event() : base_event(EVENT_REPAIR) {}

        size_t get_serialized_size() override
        {
            document.clear();
            document["type"] = type;
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, target_sn_len);
            if (comment != nullptr && comment_len == 0) {
                document["comment"] = ArduinoJson::MsgPackBinary(comment, comment_len);
            }

            return ArduinoJson::measureMsgPack(document);
        }

        size_t serialize(uint8_t *buf_out, size_t buf_size) override
        {
            document.clear();
            document["type"] = type;
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
        dispose_event() : base_event(EVENT_DISPOSE) {}

        size_t get_serialized_size() override
        {
            document.clear();
            document["type"] = type;
            document["sn"] = ArduinoJson::MsgPackBinary(target_sn, target_sn_len);
            if (comment != nullptr && comment_len == 0) {
                document["comment"] = ArduinoJson::MsgPackBinary(comment, comment_len);
            }

            return ArduinoJson::measureMsgPack(document);
        }

        size_t serialize(uint8_t *buf_out, size_t buf_size) override
        {
            document.clear();
            document["type"] = type;
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