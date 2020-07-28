#include "Uart.h"
#include <errno.h>

int UartOpen(UART_Id uartId, int baudRate, int dataBits, UART_Parity parity, int stopBits, UART_FlowControl flowControl)
{
    UART_Config uartConfig;
    UART_InitConfig(&uartConfig);

    uartConfig.baudRate = (UART_BaudRate_Type)baudRate;
    switch (dataBits)
    {
    case 5:
        uartConfig.dataBits = UART_DataBits_Five;
        break;
    case 6:
        uartConfig.dataBits = UART_DataBits_Six;
        break;
    case 7:
        uartConfig.dataBits = UART_DataBits_Seven;
        break;
    case 8:
        uartConfig.dataBits = UART_DataBits_Eight;
        break;
    default:
        errno = EINVAL;
        return -1;
    }
    uartConfig.parity = parity;
    switch (stopBits)
    {
    case 1:
        uartConfig.stopBits = UART_StopBits_One;
        break;
    case 2:
        uartConfig.stopBits = UART_StopBits_Two;
        break;
    default:
        errno = EINVAL;
        return -1;
    }
    uartConfig.flowControl = flowControl;

    return UART_Open(uartId, &uartConfig);
}
