#include "flash_algo_character_cb.hpp"

void flash_algo_character_cb::onRead(NimBLECharacteristic *pCharacteristic)
{
    NimBLECharacteristicCallbacks::onRead(pCharacteristic);
}

void flash_algo_character_cb::on_merged_packet()
{

}
