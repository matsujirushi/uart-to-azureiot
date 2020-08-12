// Copyright (c) 2020 matsujirushi
// Released under the MIT license
// https://github.com/matsujirushi/uart-to-azureiot/blob/master/LICENSE.txt

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "Exit.h"
#include "Termination.h"
#include "Gpio.h"

#include <applibs/uart.h>
#include <applibs/gpio.h>
#include <applibs/log.h>
#include <applibs/eventloop.h>

#include "../../hardware-definitions/inc/hw/uart-to-azureiot.h"

#include "eventloop_timer_utilities.h"

const size_t UART_RECEIVE_BUFFER_SIZE = 256;

// Exit Code for Application
typedef enum {
    ExitCode_Success                = 0,
    ExitCode_SigTerm                = 1,

    ExitCode_Init_OpenButton        = 2,
    ExitCode_Init_UartOpen          = 3,
    ExitCode_Init_EventLoop         = 4,
    ExitCode_Init_ButtonPollTimer   = 5,
    ExitCode_Init_RegisterIo        = 6,

    ExitCode_Main_EventLoopFail     = 7,

    ExitCode_ButtonTimer_Consume    = 8,
    ExitCode_SendMessage_Write      = 9,
    ExitCode_UartEvent_Read         = 10,
} ExitCode;

static int UartFd = -1;             // fd

static int ButtonFd = -1;           // fd
static bool ButtonValue = false;

static EventLoop* EvLoop = NULL;
static EventRegistration* EvRegUart = NULL;
static EventLoopTimer* EvTimerButton = NULL;

static void SendUartMessage(int uartFd, const char* dataToSend)
{
    size_t totalBytesSent = 0;
    const size_t totalBytesToSend = strlen(dataToSend);
    int sendIterations = 0;
    while (totalBytesSent < totalBytesToSend) {
        sendIterations++;

        size_t bytesLeftToSend = totalBytesToSend - totalBytesSent;
        const char* remainingMessageToSend = dataToSend + totalBytesSent;
        ssize_t bytesSent = write(uartFd, remainingMessageToSend, bytesLeftToSend);
        if (bytesSent == -1) {
            Exit_DoExitWithLog(ExitCode_SendMessage_Write, "ERROR: write() - %s (%d)\n", strerror(errno), errno);
            return;
        }

        totalBytesSent += (size_t)bytesSent;
    }

    Log_Debug("Sent %zu bytes over UART in %d calls.\n", totalBytesSent, sendIterations);
}

static void ButtonTimerEventHandler(EventLoopTimer* timer)
{
    if (ConsumeEventLoopTimerEvent(timer) == -1) {
        Exit_DoExit(ExitCode_ButtonTimer_Consume);
        return;
    }

    const bool newButtonValue = GpioReadInv(ButtonFd);
    if (!ButtonValue && newButtonValue) SendUartMessage(UartFd, "Hello world!\n");
    ButtonValue = newButtonValue;
}

static void UartEventHandler(EventLoop* el, int fd, EventLoop_IoEvents events, void* context)
{
    uint8_t receiveBuffer[UART_RECEIVE_BUFFER_SIZE + 1];
    ssize_t bytesRead;

    bytesRead = read(UartFd, receiveBuffer, UART_RECEIVE_BUFFER_SIZE);
    if (bytesRead == -1) {
        Exit_DoExitWithLog(ExitCode_UartEvent_Read, "ERROR: read() - %s (%d)\n", strerror(errno), errno);
        return;
    }

    if (bytesRead > 0) {
        receiveBuffer[bytesRead] = 0;
        Log_Debug("UART received %d bytes: '%s'.\n", bytesRead, (char*)receiveBuffer);
    }
}

static void InitPeripheralsAndHandlers(void)
{
    Termination_SetExitCode(ExitCode_SigTerm);

    ButtonFd = GPIO_OpenAsInput(SEND_BUTTON);
    if (ButtonFd == -1) {
        Exit_DoExitWithLog(ExitCode_Init_OpenButton, "ERROR: GPIO_OpenAsInput() - %s (%d)\n", strerror(errno), errno);
        return;
    }
    UART_Config uartConfig;
    UART_InitConfig(&uartConfig);
    uartConfig.baudRate = 115200;
    uartConfig.flowControl = UART_FlowControl_None;
    UartFd = UART_Open(ARDUINO_UART, &uartConfig);
    if (UartFd == -1) {
        Exit_DoExitWithLog(ExitCode_Init_UartOpen, "ERROR: UART_Open() - %s (%d)\n", strerror(errno), errno);
        return;
    }

    EvLoop = EventLoop_Create();
    if (EvLoop == NULL) {
        Exit_DoExitWithLog(ExitCode_Init_EventLoop, "ERROR: EventLoop_Create() - %s (%d)\n", strerror(errno), errno);
        return;
    }
    const struct timespec buttonPressCheckPeriod1Ms = { .tv_sec = 0, .tv_nsec = 1000 * 1000 };
    EvTimerButton = CreateEventLoopPeriodicTimer(EvLoop, ButtonTimerEventHandler, &buttonPressCheckPeriod1Ms);
    if (EvTimerButton == NULL) {
        Exit_DoExitWithLog(ExitCode_Init_ButtonPollTimer, "ERROR: CreateEventLoopPeriodicTimer() - %s (%d)\n", strerror(errno), errno);
        return;
    }
    EvRegUart = EventLoop_RegisterIo(EvLoop, UartFd, EventLoop_Input, UartEventHandler, NULL);
    if (EvRegUart == NULL) {
        Exit_DoExitWithLog(ExitCode_Init_RegisterIo, "ERROR: EventLoop_RegisterIo() - %s (%d)\n", strerror(errno), errno);
        return;
    }
}

static void ClosePeripheralsAndHandlers(void)
{
    EventLoop_UnregisterIo(EvLoop, EvRegUart);
    DisposeEventLoopTimer(EvTimerButton);
    EventLoop_Close(EvLoop);

    if (UartFd >= 0) close(UartFd);
    if (ButtonFd >= 0) close(UartFd);
}

int main(int argc, char* argv[])
{
    Log_Debug("Application starting.\n");
    InitPeripheralsAndHandlers();

    while (!Exit_IsExit()) {
        const EventLoop_Run_Result result = EventLoop_Run(EvLoop, -1, true);
        if (result == EventLoop_Run_Failed && errno != EINTR) Exit_DoExit(ExitCode_Main_EventLoopFail);
    }

    Log_Debug("Application exiting.\n");
    ClosePeripheralsAndHandlers();

    const int exitCode = Exit_GetExitCode();
    Log_Debug("Exit Code is %d.\n", exitCode);

    return exitCode;
}
