#pragma once

#include <cstdint>
#include <cstddef>
#include <esp_err.h>

#include <ArduinoJson.hpp>
#include <psram_json_allocator.hpp>

namespace rpc::cmd
{
    struct base_cmd
    {
    protected:
        PsRamAllocator allocator;
        ArduinoJson::JsonDocument document;

    public:
        virtual size_t serialize(uint8_t *buf_out, size_t buf_size) = 0;
        virtual size_t get_serialized_size() = 0;
    };
}