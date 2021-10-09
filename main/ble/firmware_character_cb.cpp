#include "firmware_character_cb.hpp"

void firmware_character_cb::onRead(NimBLECharacteristic *pCharacteristic)
{
    NimBLECharacteristicCallbacks::onRead(pCharacteristic);
}

void firmware_character_cb::onWrite(NimBLECharacteristic *pCharacteristic)
{
    NimBLECharacteristicCallbacks::onWrite(pCharacteristic);
}
