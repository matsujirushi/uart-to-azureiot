# Copyright (c) 2020 matsujirushi
# Released under the MIT license
# https://github.com/matsujirushi/uart-to-azureiot/blob/master/LICENSE.txt

cmake_minimum_required(VERSION 3.10)

project(uart-to-azureiot-hlapp C)

azsphere_configure_tools(TOOLS_REVISION "20.07")
azsphere_configure_api(TARGET_API_SET "6")

add_compile_options(-DNETWORKING_STRUCTS_VERSION=1)
add_compile_options(-DWIFICONFIG_STRUCTS_VERSION=1)
add_compile_options(-DUART_STRUCTS_VERSION=1)
add_compile_options(-DSPI_STRUCTS_VERSION=1)

# Libraries
add_subdirectory(../../lib/eventloop_timer_utilities lib/eventloop_timer_utilities)
add_subdirectory(../../lib/parson lib/parson)

# Executable
add_executable(${PROJECT_NAME} main.c Exit.c Termination.c Gpio.c BytesSpan.c MessageParser.c  AzureDeviceClient.c)
target_link_libraries(${PROJECT_NAME} azureiot applibs pthread gcc_s c eventloop_timer_utilities parson)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../hardware-definitions" TARGET_DEFINITION "uart-to-azureiot.json")
azsphere_target_add_image_package(${PROJECT_NAME})
