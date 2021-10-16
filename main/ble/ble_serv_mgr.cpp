#include <esp_log.h>
#include <NimBLEDevice.h>
#include <manifest_character_cb.hpp>
#include <firmware_character_cb.hpp>
#include <flash_algo_character_cb.hpp>
#include <state_character_cb.hpp>
#include "ble_serv_mgr.hpp"

esp_err_t ble_serv_mgr::init()
{
    NimBLEDevice::init("soul-injector");
    gatt_server = NimBLEDevice::createServer();
    if (gatt_server == nullptr) {
        ESP_LOGE(TAG, "Failed to initialise GATT server");
        return ESP_ERR_NO_MEM;
    }

    soul_service = gatt_server->createService(SOUL_SERVICE_UUID);
    if (soul_service == nullptr) {
        ESP_LOGE(TAG, "Failed to init SoulInjector service");
        return ESP_ERR_NO_MEM;
    }

    manifest_char = soul_service->createCharacteristic(MANIFEST_CHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC);
    if (manifest_char == nullptr) {
        ESP_LOGE(TAG, "Failed to init manifest characteristic");
        return ESP_ERR_NO_MEM;
    }
    manifest_char->setCallbacks(new manifest_character_cb());

    firmware_char = soul_service->createCharacteristic(FIRMWARE_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC);
    if (firmware_char == nullptr) {
        ESP_LOGE(TAG, "Failed to init firmware characteristic");
        return ESP_ERR_NO_MEM;
    }
    firmware_char->setCallbacks(new firmware_character_cb());

    flash_algo_char = soul_service->createCharacteristic(FLASH_ALGO_CHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC);
    if (flash_algo_char == nullptr) {
        ESP_LOGE(TAG, "Failed to init flash algo characteristic");
        return ESP_ERR_NO_MEM;
    }
    flash_algo_char->setCallbacks(new flash_algo_character_cb());

    state_ctrl_char = soul_service->createCharacteristic(STATE_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC);
    if (state_ctrl_char == nullptr) {
        ESP_LOGE(TAG, "Failed to init state ctrl characteristic");
        return ESP_ERR_NO_MEM;
    }
    state_ctrl_char->setCallbacks(new state_character_cb());

    if (!soul_service->start()) {
        ESP_LOGE(TAG, "Failed to start SoulInjector service");
        return ESP_FAIL;
    }

    gatt_server->getAdvertising()->addServiceUUID(SOUL_SERVICE_UUID);
    if (!gatt_server->getAdvertising()->start()) {
        ESP_LOGE(TAG, "Failed to start GATT Server advertising");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void ble_serv_mgr::onConnect(NimBLEServer *server)
{
    NimBLEServerCallbacks::onConnect(server);
}

void ble_serv_mgr::onDisconnect(NimBLEServer *server)
{
    NimBLEServerCallbacks::onDisconnect(server);
}

uint32_t ble_serv_mgr::onPassKeyRequest()
{
    return NimBLEServerCallbacks::onPassKeyRequest();
}

bool ble_serv_mgr::onConfirmPIN(uint32_t pass_key)
{
    return NimBLEServerCallbacks::onConfirmPIN(pass_key);
}

void ble_serv_mgr::onAuthenticationComplete(ble_gap_conn_desc *desc)
{
    NimBLEServerCallbacks::onAuthenticationComplete(desc);
}
