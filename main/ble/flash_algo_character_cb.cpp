#include "flash_algo_character_cb.hpp"

void flash_algo_character_cb::onRead(NimBLECharacteristic *pCharacteristic)
{
    NimBLECharacteristicCallbacks::onRead(pCharacteristic);
}

void flash_algo_character_cb::onWrite(NimBLECharacteristic *pCharacteristic)
{
    NimBLECharacteristicCallbacks::onWrite(pCharacteristic);
}
