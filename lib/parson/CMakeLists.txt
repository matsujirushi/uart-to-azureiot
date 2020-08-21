# Copyright (c) 2020 matsujirushi
# Released under the MIT license
# https://github.com/matsujirushi/uart-to-azureiot/blob/master/LICENSE.txt

add_library(parson src/parson.c)

target_include_directories(parson PUBLIC include)
