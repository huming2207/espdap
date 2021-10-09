#pragma once

#include <esp_err.h>
#include <NimBLEServer.h>

class ble_serv_mgr : public NimBLEServerCallbacks
{
public:
    static ble_serv_mgr& instance()
    {
        static ble_serv_mgr instance;
        return instance;
    }

    ble_serv_mgr(ble_serv_mgr const &) = delete;
    void operator=(ble_serv_mgr const &) = delete;

private:
    ble_serv_mgr() = default;
    NimBLEServer *gatt_server = nullptr;
    NimBLEService *soul_service = nullptr;
    NimBLECharacteristic *manifest_char = nullptr;
    NimBLECharacteristic *flash_algo_char = nullptr;
    NimBLECharacteristic *firmware_char = nullptr;
    NimBLECharacteristic *state_ctrl_char = nullptr;
    static constexpr const char *TAG = "soul_gatt_serv";
    static constexpr const char *SOUL_SERVICE_UUID = "f210ebaa-2f9c-2611-8a37-693fe21f2100";
    static constexpr const char *MANIFEST_CHAR_UUID = "f210ebaa-2f9c-2611-8a37-693fe21f2110";
    static constexpr const char *FLASH_ALGO_CHAR_UUID = "f210ebaa-2f9c-2611-8a37-693fe21f2111";
    static constexpr const char *FIRMWARE_UUID = "f210ebaa-2f9c-2611-8a37-693fe21f2112";
    static constexpr const char *STATE_UUID = "f210ebaa-2f9c-2611-8a37-693fe21f2113";

public:
    esp_err_t init();

private:
    void onConnect(NimBLEServer *server) override;
    void onDisconnect(NimBLEServer* server) override;
    uint32_t onPassKeyRequest() override;
    bool onConfirmPIN(uint32_t pass_key) override;
    void onAuthenticationComplete(ble_gap_conn_desc *desc) override;
};
