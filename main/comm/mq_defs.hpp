#pragma once

#define static_char static const constexpr char

namespace mq
{

    static_char TOPIC_REPORT_BASE[] = "/soulinjector/v1/report";
    static_char TOPIC_REPORT_INIT[] = "init";
    static_char TOPIC_REPORT_HOST_STATE[] = "state";
    static_char TOPIC_REPORT_PROG[] = "prog";
    static_char TOPIC_REPORT_SELF_TEST[] = "test/int";
    static_char TOPIC_REPORT_EXTN_TEST[] = "test/ext";
    static_char TOPIC_REPORT_ERASE[] = "erase";
    static_char TOPIC_REPORT_REPAIR[] = "repair";
    static_char TOPIC_REPORT_DISPOSE[] = "dispose";

    static_char TOPIC_CMD_BASE[] = "/soulinjector/v1/cmd";
    static_char TOPIC_CMD_METADATA_FIRMWARE[] = "meta/fw";
    static_char TOPIC_CMD_METADATA_FLASH_ALGO[] = "meta/algo";
    static_char TOPIC_CMD_BIN_FIRMWARE[] = "bin/fw";
    static_char TOPIC_CMD_BIN_FLASH_ALGO[] = "bin/algo";
    static_char TOPIC_CMD_SET_STATE[] = "state";
    static_char TOPIC_CMD_READ_MEM[] = "read_mem";

    enum state : uint32_t {
        MQ_STATE_PING = 0,
        MQ_STATE_PONG = 0x01,

        MQ_STATE_EXEC_TARGET_RESET = 0x100,
        MQ_STATE_EXEC_TARGET_IDENT = 0x110,

        MQ_STATE_EXEC_FLASH_ERASE_FULL = 0x200,
        MQ_STATE_EXEC_FLASH_ERASE_RANGE = 0x201,

        MQ_STATE_EXEC_FLASH_PROG_FIRMWARE = 0x300,
        MQ_STATE_EXEC_FLASH_PROG_BLOB = 0x301,

        MQ_STATE_EXEC_FLASH_VERIFY_FULL = 0x400,
        MQ_STATE_EXEC_FLASH_VERIFY_RANGE = 0x401,

        MQ_STATE_EXEC_TEST_SELF = 0x1000,

        MQ_STATE_EXEC_TEST_EXTERN = 0x2000,
        MQ_STATE_EXEC_TEST_POWER_CONSUMPTION = 0x2001,

        MQ_STATE_READ_MEMORY = 0x3000,
        MQ_STATE_WRITE_MEMORY = 0x4000,

        MQ_STATE_DISPLAY_SUCCESS_MESSAGE = 0x5000,
        MQ_STATE_DISPLAY_INFO_MESSAGE = 0x5001,
        MQ_STATE_DISPLAY_WARNING_MESSAGE = 0x5002,
        MQ_STATE_DISPLAY_ERROR_MESSAGE = 0x5003,


        MQ_STATE_HOST_REBOOT = 0xffff0000,
        MQ_STATE_HOST_OTA = 0xffff0010,
        MQ_STATE_HOST_LOAD_NEW_CFG = 0xffff0020,
    };
}

#undef static_char