#include "state_character_cb.hpp"

void state_character_cb::onRead(NimBLECharacteristic *pCharacteristic)
{
    NimBLECharacteristicCallbacks::onRead(pCharacteristic);
}

void state_character_cb::on_merged_packet()
{

}
