#pragma once

#include <NimBLEDevice.h>
#include "chunked_characteristic.hpp"

class state_character_cb : public chunked_characteristic
{
public:
    void onRead(NimBLECharacteristic* pCharacteristic) override;
    void on_merged_packet() override;

private:
    static constexpr const char *TAG = "state_char_cb";
};
