#include "manifest_character_cb.hpp"

void manifest_character_cb::onRead(NimBLECharacteristic *pCharacteristic)
{
    NimBLECharacteristicCallbacks::onRead(pCharacteristic);
}

void manifest_character_cb::onWrite(NimBLECharacteristic *pCharacteristic)
{
    NimBLECharacteristicCallbacks::onWrite(pCharacteristic);
}
