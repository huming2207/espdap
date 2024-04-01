#pragma once

namespace rpc
{
    struct __attribute__((packed)) prog_event
    {
        uint32_t project_id;
        uint32_t addr;
        uint32_t len;
        char target_sn[32];
    };

    struct __attribute__((packed)) self_test_event
    {
        uint32_t test_id;
        uint32_t return_num;
        char test_name[32];
        char target_sn[32];
    };

    struct __attribute__((packed)) erase_event
    {
        uint32_t addr;
        uint32_t len;
        char target_sn[32];
    };

    struct __attribute__((packed)) repair_event
    {
        char target_sn[32];
    };


    struct __attribute__((packed)) dispose_event
    {
        char target_sn[32];
    };
}