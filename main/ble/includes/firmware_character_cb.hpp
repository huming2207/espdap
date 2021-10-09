#pragma once

#include <NimBLEDevice.h>

class firmware_character_cb : public NimBLECharacteristicCallbacks
{
public:
    void onRead(NimBLECharacteristic* pCharacteristic) override;
    void onWrite(NimBLECharacteristic* pCharacteristic) override;

private:
    static constexpr const char *TAG = "fw_char";
};
