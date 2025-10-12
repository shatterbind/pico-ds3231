# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
option(BUILD_EXAMPLE "Build the standalone example for this library" ON)


set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(ds3231_pico)

pico_sdk_init()

add_subdirectory(src)
include_directories(src)
add_executable(ds3231_pico examples/main.c)

pico_enable_stdio_usb(ds3231_pico 1)
pico_enable_stdio_uart(ds3231_pico 1)

pico_add_extra_outputs(ds3231_pico)

target_link_libraries(ds3231_pico ds3231 hardware_i2c pico_stdlib)
