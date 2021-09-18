idf_component_register(
        SRCS
            "main.cpp"
            "prog/swd_prog.cpp" "prog/includes/swd_prog.hpp"
            "driver/swd_driver.cpp" "driver/includes/swd_driver.hpp"
        INCLUDE_DIRS
            "." "misc/includes" "prog/includes" "driver/includes"
        PRIV_REQUIRES
            "daplink-esp"
            "mbedtls"
)
