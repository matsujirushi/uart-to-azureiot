#pragma once

#include <applibs/uart.h>

int UartOpen(UART_Id uartId, int baudRate, int dataBits, UART_Parity parity, int stopBits, UART_FlowControl flowControl);
