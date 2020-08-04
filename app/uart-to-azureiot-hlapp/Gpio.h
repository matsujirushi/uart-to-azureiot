#pragma once

#include <stdbool.h>
#include <applibs/gpio.h>

bool GpioRead(int gpioFd);
bool GpioReadInv(int gpioFd);
