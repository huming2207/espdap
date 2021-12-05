#include "manifest_character_cb.hpp"

void manifest_character_cb::onRead(NimBLECharacteristic *pCharacteristic)
{
    NimBLECharacteristicCallbacks::onRead(pCharacteristic);
}

void manifest_character_cb::on_merged_packet()
{

}
