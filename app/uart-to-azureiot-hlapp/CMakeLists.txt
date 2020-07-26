﻿cmake_minimum_required(VERSION 3.10)

project(uart-to-azureiot-hlapp C)

azsphere_configure_tools(TOOLS_REVISION "20.04")
azsphere_configure_api(TARGET_API_SET "5")

# Libraries
add_subdirectory(../../lib/eventloop_timer_utilities lib/eventloop_timer_utilities)

# Executable
add_executable(${PROJECT_NAME} main.c Exit.c Termination.c)
target_link_libraries(${PROJECT_NAME} applibs pthread gcc_s c eventloop_timer_utilities)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../azure-sphere-samples-hardware/mt3620_rdb" TARGET_DEFINITION "sample_hardware.json")
azsphere_target_add_image_package(${PROJECT_NAME})