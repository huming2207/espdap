# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set (COMPONENTS ${IDF_TARGET} main freertos esptool_py wamr)
set (WAMR_BUILD_LIBC_WASI 0)
set (WAMR_BUILD_AOT 0)
set (WAMR_BUILD_JIT 0)
set (WAMR_BUILD_LAZY_JIT 0)
set (WAMR_BUILD_FAST_INTERP 1)
set (WAMR_APP_THREAD_STACK_SIZE_MAX 65536)
list(APPEND EXTRA_COMPONENT_DIRS "./wamr/build-scripts/esp-idf")

project(soulinjector)
