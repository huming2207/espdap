idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "." "misc/includes"
        PRIV_REQUIRES
            "daplink-esp"
            "mbedtls"
                    )
