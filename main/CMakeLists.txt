idf_component_register(
        SRCS
            "main.cpp"
            "prog/swd_prog.cpp" "prog/includes/swd_prog.hpp"
            "prog/firmware_manager.cpp" "prog/includes/firmware_manager.hpp"
            "prog/manifest_mgr.cpp" "prog/includes/manifest_mgr.hpp"
            "prog/swd_headless_flasher.cpp" "prog/includes/swd_headless_flasher.hpp"
            "external/includes/ArduinoJson.hpp" "prog/includes/led_ctrl.hpp"
            "misc/includes/file_utils.hpp"
            "comm/ble/manifest_character_cb.cpp" "comm/ble/includes/manifest_character_cb.hpp"
            "comm/ble/flash_algo_character_cb.cpp" "comm/ble/includes/flash_algo_character_cb.hpp"
            "comm/ble/firmware_character_cb.cpp" "comm/ble/includes/firmware_character_cb.hpp"
            "comm/ble/state_character_cb.cpp" "comm/ble/includes/state_character_cb.hpp"
            "comm/ble/ble_serv_mgr.cpp" "comm/ble/includes/ble_serv_mgr.hpp"
            "comm/ble/includes/chunked_characteristic.hpp"
            "comm/usb/cdc_acm.cpp" "comm/usb/includes/cdc_acm.hpp"
            "misc/includes/soul_nvs.hpp"
        INCLUDE_DIRS
            "." "misc/includes" "prog/includes" "external/includes" "comm/ble/includes" "comm/usb/includes"
        PRIV_REQUIRES
            "daplink-esp"
            "mbedtls"
            "spiffs"
            "esp-nimble-cpp"
            "tinyusb"
        EMBED_FILES
            "resources/stm32l0x1.bin"
)

spiffs_create_partition_image(storage fs_sample FLASH_IN_PROJECT)