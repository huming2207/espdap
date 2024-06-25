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
}

#undef static_char