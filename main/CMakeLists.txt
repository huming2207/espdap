idf_component_register(
        SRCS
            "main.cpp"
            "prog/swd_prog.cpp" "prog/includes/swd_prog.hpp"
            "prog/flash_algo.cpp" "prog/includes/flash_algo.hpp"
            "prog/manifest_mgr.cpp" "prog/includes/manifest_mgr.hpp"
            "prog/swd_headless_flasher.cpp" "prog/includes/swd_headless_flasher.hpp"
            "external/includes/ArduinoJson.hpp" "prog/includes/led_ctrl.hpp"
            "misc/includes/file_utils.hpp"
            "ble/manifest_character_cb.cpp" "ble/includes/manifest_character_cb.hpp"
            "ble/flash_algo_character_cb.cpp" "ble/includes/flash_algo_character_cb.hpp"
            "ble/firmware_character_cb.cpp" "ble/includes/firmware_character_cb.hpp"
            "ble/state_character_cb.cpp" "ble/includes/state_character_cb.hpp"
            "ble/ble_serv_mgr.cpp" "ble/includes/ble_serv_mgr.hpp"
            "ble/includes/chunked_characteristic.hpp"
            "misc/includes/soul_nvs.hpp"
        INCLUDE_DIRS
            "." "misc/includes" "prog/includes" "external/includes" "ble/includes"
        PRIV_REQUIRES
            "daplink-esp"
            "mbedtls"
            "spiffs"
            "esp-nimble-cpp"
)

spiffs_create_partition_image(storage fs_sample FLASH_IN_PROJECT)