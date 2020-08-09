﻿/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

   // This sample C application for Azure Sphere demonstrates how to use a UART (serial port).
   // The sample opens a UART with a baud rate of 115200. Pressing a button causes characters
   // to be sent from the device over the UART; data received by the device from the UART is echoed to
   // the Visual Studio Output Window.
   //
   // It uses the API for the following Azure Sphere application libraries:
   // - UART (serial port)
   // - GPIO (digital input for button)
   // - log (messages shown in Visual Studio's Device Output window during debugging)
   // - eventloop (system invokes handlers for timer events)

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "Exit.h"
#include "Termination.h"
#include "Uart.h"
#include "Gpio.h"

#include <applibs/uart.h>
#include <applibs/gpio.h>
#include <applibs/log.h>
#include <applibs/eventloop.h>

// By default, this sample targets hardware that follows the MT3620 Reference
// Development Board (RDB) specification, such as the MT3620 Dev Kit from
// Seeed Studio.
//
// To target different hardware, you'll need to update CMakeLists.txt. See
// https://github.com/Azure/azure-sphere-samples/tree/master/Hardware for more details.
//
// This #include imports the sample_hardware abstraction from that hardware definition.
#include "../../hardware-definitions/inc/hw/uart-to-azureiot.h"

#include "eventloop_timer_utilities.h"

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,
    ExitCode_TermHandler_SigTerm = 1,
    ExitCode_SendMessage_Write = 2,
    ExitCode_ButtonTimer_Consume = 3,
    ExitCode_ButtonTimer_GetValue = 4,
    ExitCode_UartEvent_Read = 5,
    ExitCode_Init_EventLoop = 6,
    ExitCode_Init_UartOpen = 7,
    ExitCode_Init_RegisterIo = 8,
    ExitCode_Init_OpenButton = 9,
    ExitCode_Init_ButtonPollTimer = 10,
    ExitCode_Main_EventLoopFail = 11
} ExitCode;

// File descriptors - initialized to invalid value
static int uartFd = -1;
static int gpioButtonFd = -1;

EventLoop* eventLoop = NULL;
EventRegistration* uartEventReg = NULL;
EventLoopTimer* buttonPollTimer = NULL;

// State variables
static bool buttonState = false;

static void SendUartMessage(int uartFd, const char* dataToSend);
static void ButtonTimerEventHandler(EventLoopTimer* timer);
static void UartEventHandler(EventLoop* el, int fd, EventLoop_IoEvents events, void* context);
static void CloseFdAndPrintError(int fd, const char* fdName);

/// <summary>
///     Helper function to send a fixed message via the given UART.
/// </summary>
/// <param name="uartFd">The open file descriptor of the UART to write to</param>
/// <param name="dataToSend">The data to send over the UART</param>
static void SendUartMessage(int uartFd, const char* dataToSend)
{
    size_t totalBytesSent = 0;
    size_t totalBytesToSend = strlen(dataToSend);
    int sendIterations = 0;
    while (totalBytesSent < totalBytesToSend) {
        sendIterations++;

        // Send as much of the remaining data as possible
        size_t bytesLeftToSend = totalBytesToSend - totalBytesSent;
        const char* remainingMessageToSend = dataToSend + totalBytesSent;
        ssize_t bytesSent = write(uartFd, remainingMessageToSend, bytesLeftToSend);
        if (bytesSent == -1) {
            Exit_DoExitWithLog(ExitCode_SendMessage_Write, "ERROR: Could not write to UART: %s (%d).\n", strerror(errno), errno);
            return;
        }

        totalBytesSent += (size_t)bytesSent;
    }

    Log_Debug("Sent %zu bytes over UART in %d calls.\n", totalBytesSent, sendIterations);
}

/// <summary>
///     Handle button timer event: if the button is pressed, send data over the UART.
/// </summary>
static void ButtonTimerEventHandler(EventLoopTimer* timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        Exit_DoExit(ExitCode_ButtonTimer_Consume);
        return;
    }

    // Check for a button press
    bool newButtonState = GpioReadInv(gpioButtonFd);
    // If the button has just been pressed, send data over the UART
    // The button has GPIO_Value_Low when pressed and GPIO_Value_High when released
    if (newButtonState != buttonState) {
        if (newButtonState) {
            SendUartMessage(uartFd, "Hello world!\n");
        }
        buttonState = newButtonState;
    }
}

/// <summary>
///     Handle UART event: if there is incoming data, print it.
///     This satisfies the EventLoopIoCallback signature.
/// </summary>
static void UartEventHandler(EventLoop* el, int fd, EventLoop_IoEvents events, void* context)
{
    const size_t receiveBufferSize = 256;
    uint8_t receiveBuffer[receiveBufferSize + 1]; // allow extra byte for string termination
    ssize_t bytesRead;

    // Read incoming UART data. It is expected behavior that messages may be received in multiple
    // partial chunks.
    bytesRead = read(uartFd, receiveBuffer, receiveBufferSize);
    if (bytesRead == -1) {
        Exit_DoExitWithLog(ExitCode_UartEvent_Read, "ERROR: Could not read UART: %s (%d).\n", strerror(errno), errno);
        return;
    }

    if (bytesRead > 0) {
        // Null terminate the buffer to make it a valid string, and print it
        receiveBuffer[bytesRead] = 0;
        Log_Debug("UART received %d bytes: '%s'.\n", bytesRead, (char*)receiveBuffer);
    }
}

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
static void InitPeripheralsAndHandlers(void)
{
    Termination_SetExitCode(ExitCode_TermHandler_SigTerm);

    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Exit_DoExitWithLog(ExitCode_Init_EventLoop, "Could not create event loop.\n");
        return;
    }

    uartFd = UartOpen(MT3620_RDB_HEADER2_ISU0_UART, 115200, 8, UART_Parity_None, 1, UART_FlowControl_None);
    if (uartFd == -1) {
        Exit_DoExitWithLog(ExitCode_Init_UartOpen, "ERROR: Could not open UART: %s (%d).\n", strerror(errno), errno);
        return;
    }
    uartEventReg = EventLoop_RegisterIo(eventLoop, uartFd, EventLoop_Input, UartEventHandler, NULL);
    if (uartEventReg == NULL) {
        Exit_DoExit(ExitCode_Init_RegisterIo);
        return;
    }

    // Open SAMPLE_BUTTON_1 GPIO as input, and set up a timer to poll it
    Log_Debug("Opening MT3620_RDB_BUTTON_A as input.\n");
    gpioButtonFd = GPIO_OpenAsInput(MT3620_RDB_BUTTON_A);
    if (gpioButtonFd == -1) {
        Exit_DoExitWithLog(ExitCode_Init_OpenButton, "ERROR: Could not open button GPIO: %s (%d).\n", strerror(errno), errno);
        return;
    }
    struct timespec buttonPressCheckPeriod1Ms = { .tv_sec = 0, .tv_nsec = 1000 * 1000 };
    buttonPollTimer = CreateEventLoopPeriodicTimer(eventLoop, ButtonTimerEventHandler, &buttonPressCheckPeriod1Ms);
    if (buttonPollTimer == NULL) {
        Exit_DoExit(ExitCode_Init_ButtonPollTimer);
        return;
    }
}

/// <summary>
///     Closes a file descriptor and prints an error on failure.
/// </summary>
/// <param name="fd">File descriptor to close</param>
/// <param name="fdName">File descriptor name to use in error message</param>
static void CloseFdAndPrintError(int fd, const char* fdName)
{
    if (fd >= 0) {
        int result = close(fd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", fdName, strerror(errno), errno);
        }
    }
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    DisposeEventLoopTimer(buttonPollTimer);
    EventLoop_UnregisterIo(eventLoop, uartEventReg);
    EventLoop_Close(eventLoop);

    Log_Debug("Closing file descriptors.\n");
    CloseFdAndPrintError(gpioButtonFd, "GpioButton");
    CloseFdAndPrintError(uartFd, "Uart");
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char* argv[])
{
    Log_Debug("UART application starting.\n");
    InitPeripheralsAndHandlers();

    // Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens
    while (!Exit_IsExit()) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) Exit_DoExit(ExitCode_Main_EventLoopFail);
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting.\n");
    return Exit_GetExitCode();
}
