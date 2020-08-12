# Copyright (c) 2020 matsujirushi
# Released under the MIT license
# https://github.com/matsujirushi/uart-to-azureiot/blob/master/LICENSE.txt

add_library(eventloop_timer_utilities src/eventloop_timer_utilities.c)

target_include_directories(eventloop_timer_utilities PUBLIC include)
