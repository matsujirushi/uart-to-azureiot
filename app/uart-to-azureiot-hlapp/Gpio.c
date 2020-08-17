// Copyright (c) 2020 matsujirushi
// Released under the MIT license
// https://github.com/matsujirushi/uart-to-azureiot/blob/master/LICENSE.txt

#include "Gpio.h"
#include <assert.h>

bool GpioRead(int gpioFd)
{
	GPIO_Value_Type value;
	const int result = GPIO_GetValue(gpioFd, &value);
	assert(result == 0);

	return value == GPIO_Value_High;
}

bool GpioReadInv(int gpioFd)
{
	GPIO_Value_Type value;
	const int result = GPIO_GetValue(gpioFd, &value);
	assert(result == 0);

	return value == GPIO_Value_Low;
}

void GpioWrite(int gpioFd, bool value)
{
	const int result = GPIO_SetValue(gpioFd, value ? GPIO_Value_High : GPIO_Value_Low);
	assert(result == 0);
}

void GpioWriteInv(int gpioFd, bool value)
{
	const int result = GPIO_SetValue(gpioFd, value ? GPIO_Value_Low : GPIO_Value_High);
	assert(result == 0);
}
