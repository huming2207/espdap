idf_component_register(
        SRCS
            "main.cpp"
            "prog/swd_prog.cpp" "prog/includes/swd_prog.hpp"
        INCLUDE_DIRS
            "." "misc/includes" "prog/includes"
        PRIV_REQUIRES
            "daplink-esp"
            "mbedtls"
                    )
