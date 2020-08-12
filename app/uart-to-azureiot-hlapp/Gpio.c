// Copyright (c) 2020 matsujirushi
// Released under the MIT license
// https://github.com/matsujirushi/uart-to-azureiot/blob/master/LICENSE.txt

#include "Gpio.h"

bool GpioRead(int gpioFd)
{
	GPIO_Value_Type value;
	const int result = GPIO_GetValue(gpioFd, &value);
	if (result != 0) return false;

	return value == GPIO_Value_High;
}

bool GpioReadInv(int gpioFd)
{
	GPIO_Value_Type value;
	const int result = GPIO_GetValue(gpioFd, &value);
	if (result != 0) return true;

	return value == GPIO_Value_Low;
}
