#pragma once

#include <NimBLEDevice.h>

class flash_algo_character_cb : public NimBLECharacteristicCallbacks
{
public:
    void onRead(NimBLECharacteristic* pCharacteristic) override;
    void onWrite(NimBLECharacteristic* pCharacteristic) override;

private:
    static constexpr const char *TAG = "flash_algo_char";
};


