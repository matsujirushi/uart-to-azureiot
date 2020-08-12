// Copyright (c) 2020 matsujirushi
// Released under the MIT license
// https://github.com/matsujirushi/uart-to-azureiot/blob/master/LICENSE.txt

#pragma once

#include <stdbool.h>
#include <applibs/gpio.h>

bool GpioRead(int gpioFd);
bool GpioReadInv(int gpioFd);
