file(GLOB_RECURSE MPY_SRC "mpy_embed/*.c")

idf_component_register(
        SRCS ${MPY_SRC}
        INCLUDE_DIRS "mpy_embed" "mpy_embed/micropython_embed" "mpy_embed/build-embed" "mpy_embed/build-embed/genhdr"
)

set(MPY_TOP ${COMPONENT_DIR}/micropython)
add_custom_command(OUTPUT "mpy_embed"
        COMMAND make -C ${COMPONENT_DIR}/mpy_embed MICROPYTHON_TOP=${MPY_TOP} -f ${MPY_TOP}/ports/embed/embed.mk
        DEPENDS ${COMPONENT_DIR}/mpy_embed/mpconfigport.h
        VERBATIM)

add_custom_target(mpy DEPENDS "mpy_embed")
add_dependencies(${COMPONENT_LIB} mpy)

set_property(DIRECTORY "${COMPONENT_DIR}" APPEND PROPERTY
        ADDITIONAL_CLEAN_FILES "mpy_embed/build-embed" "mpy_embed/micropython_embed")