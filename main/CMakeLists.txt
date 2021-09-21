idf_component_register(
        SRCS
            "main.cpp"
            "prog/swd_prog.cpp" "prog/includes/swd_prog.hpp"
            "prog/flash_algo.cpp" "prog/includes/flash_algo.hpp"
            "prog/manifest_mgr.cpp" "prog/includes/manifest_mgr.hpp"
            "driver/swd_driver.cpp" "driver/includes/swd_driver.hpp"
            "external/includes/ArduinoJson.hpp"
        INCLUDE_DIRS
            "." "misc/includes" "prog/includes" "driver/includes" "external/includes"
        PRIV_REQUIRES
            "daplink-esp"
            "mbedtls"
            "spiffs"
)
