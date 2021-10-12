#pragma once

#include <NimBLEDevice.h>
#include "merged_characteristic.hpp"

class firmware_character_cb : public merged_characteristic
{
public:
    void onRead(NimBLECharacteristic* pCharacteristic) override;
    void on_merged_packet() override;

private:
    static constexpr const char *TAG = "fw_char";
};
