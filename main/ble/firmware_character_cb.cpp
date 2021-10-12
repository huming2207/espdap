#include "firmware_character_cb.hpp"

void firmware_character_cb::onRead(NimBLECharacteristic *pCharacteristic)
{
    NimBLECharacteristicCallbacks::onRead(pCharacteristic);
}

void firmware_character_cb::on_merged_packet()
{

}