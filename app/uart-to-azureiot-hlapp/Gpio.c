#include "Gpio.h"

bool GpioRead(int gpioFd)
{
	GPIO_Value_Type value;
	int result = GPIO_GetValue(gpioFd, &value);
	if (result != 0) return false;

	return value == GPIO_Value_High;
}

bool GpioReadInv(int gpioFd)
{
	GPIO_Value_Type value;
	int result = GPIO_GetValue(gpioFd, &value);
	if (result != 0) return true;

	return value == GPIO_Value_Low;
}
