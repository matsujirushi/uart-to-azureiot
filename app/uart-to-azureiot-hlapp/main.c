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
#include <assert.h>
#include "Exit.h"
#include "Termination.h"
#include "Gpio.h"
#include "MessageParser.h"
#include "AzureDeviceClient.h"

#include <applibs/uart.h>
#include <applibs/gpio.h>
#include <applibs/log.h>
#include <applibs/eventloop.h>
#include <applibs/networking.h>

#include "../../hardware-definitions/inc/hw/uart-to-azureiot.h"

#include "eventloop_timer_utilities.h"

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

static const char* IoTHubHostName = "matsujirushi-iothub.azure-devices.net";
static const char* DeviceId = "961b0f3af5c4ea9581512975f8e21a81dfed93bef7a73854d802c8bdeff7f5a8516639b653e6f082009f5c660c9b96bb1b16f49a56d7de51a089ac01ae3376ec";
static AzureDeviceClient_t* DeviceClient;

static EventLoop* EvLoop = NULL;
static EventRegistration* EvRegUart = NULL;
static EventLoopTimer* EvTimerButton = NULL;

////////////////////////////////////////////////////////////////////////////////
// Uart Send

static void UartSend(int uartFd, const char* dataToSend)
{
    size_t totalBytesSent = 0;
    const size_t totalBytesToSend = strlen(dataToSend);
    int sendIterations = 0;
    while (totalBytesSent < totalBytesToSend) {
        sendIterations++;

        size_t bytesLeftToSend = totalBytesToSend - totalBytesSent;
        const char* remainingMessageToSend = dataToSend + totalBytesSent;
        const ssize_t bytesSent = write(uartFd, remainingMessageToSend, bytesLeftToSend);
        if (bytesSent == -1) {
            Exit_DoExitWithLog(ExitCode_SendMessage_Write, "ERROR: write() - %s (%d)\n", strerror(errno), errno);
            return;
        }

        totalBytesSent += (size_t)bytesSent;
    }

    Log_Debug("Sent %zu bytes in %d calls.\n", totalBytesSent, sendIterations);
}

static void ButtonTimerEventHandler(EventLoopTimer* timer)
{
    if (ConsumeEventLoopTimerEvent(timer) == -1) {
        Exit_DoExit(ExitCode_ButtonTimer_Consume);
        return;
    }

    const bool newButtonValue = GpioReadInv(ButtonFd);
    if (!ButtonValue && newButtonValue) UartSend(UartFd, "label1:11,label2:22,3:,:4,5,:\n");
    ButtonValue = newButtonValue;
}

////////////////////////////////////////////////////////////////////////////////
// Serial Plotter Protocol

typedef struct {
    BytesSpan_t Label;
    BytesSpan_t Value;
} SpPart_t;

static bool SpIsPartSeparator(uint8_t data)
{
    return data == ' ' || data == '\t' || data == ',' ? true : false;
}

static bool SpIsLabelSeparator(uint8_t data)
{
    return data == ':' ? true : false;
}

static int SpNumberOfParts(BytesSpan_t messageSpan)
{
    if (messageSpan.Begin == messageSpan.End) return 0;

    int number = 0;
    for (uint8_t* p = messageSpan.Begin; p != messageSpan.End; ++p) {
        if (SpIsPartSeparator(*p)) ++number;
    }

    return number + 1;
}

static BytesSpan_t SpGetPartSpan(BytesSpan_t messageSpan, BytesSpan_t* partSpan)
{
    for (uint8_t* p = messageSpan.Begin; p != messageSpan.End; ++p) {
        if (SpIsPartSeparator(*p)) {
            partSpan->Begin = messageSpan.Begin;
            partSpan->End = p;
            messageSpan.Begin = p + 1;
            return messageSpan;
        }
    }

    *partSpan = messageSpan;
    messageSpan.Begin = messageSpan.End;
    return messageSpan;
}

static SpPart_t SpPartInit(BytesSpan_t partSpan)
{
    uint8_t* labelSeparator = NULL;
    for (uint8_t* p = partSpan.Begin; p != partSpan.End; ++p) {
        if (SpIsLabelSeparator(*p)) {
            labelSeparator = p;
            break;
        }
    }

    SpPart_t part;
    if (labelSeparator != NULL) {
        part.Label.Begin = partSpan.Begin;
        part.Label.End = labelSeparator;
        part.Value.Begin = labelSeparator + 1;
        part.Value.End = partSpan.End;
    }
    else {
        part.Label.Begin = NULL;
        part.Label.End = NULL;
        part.Value = partSpan;
    }

    return part;
}

////////////////////////////////////////////////////////////////////////////////
// Uart Receive

static void MessageReceivedHandler(BytesSpan_t messageSpan)
{
    const int partNumber = SpNumberOfParts(messageSpan);
    Log_Debug("===\n");
    for (int i = 0; i < partNumber; ++i) {
        BytesSpan_t partSpan;
        messageSpan = SpGetPartSpan(messageSpan, &partSpan);
        SpPart_t part = SpPartInit(partSpan);

        Log_Debug("%d:[", i);
        for (uint8_t* p = partSpan.Begin; p != partSpan.End; ++p) Log_Debug("%c", *p);
        Log_Debug("]\n");

        if (part.Label.Begin != part.Label.End) {
            Log_Debug("  label:[");
            for (uint8_t* p = part.Label.Begin; p != part.Label.End; ++p) Log_Debug("%c", *p);
            Log_Debug("]\n");
        }
        if (part.Value.Begin != part.Value.End) {
            Log_Debug("  value:[");
            for (uint8_t* p = part.Value.Begin; p != part.Value.End; ++p) Log_Debug("%c", *p);
            Log_Debug("]\n");
        }
    }
    Log_Debug("===\n");

    JSON_Value* telemetryValue = json_value_init_object();
    JSON_Object* telemetryObject = json_value_get_object(telemetryValue);

    json_object_set_boolean(telemetryObject, "value", true);    // TODO

    bool ret = AzureDeviceClientSendTelemetryAsync(DeviceClient, telemetryObject);
    assert(ret);

    json_value_free(telemetryValue);

}

static void UartReceiveHandler(EventLoop* el, int fd, EventLoop_IoEvents events, void* context)
{
    BytesSpan_t buffer = MessageParserGetReceiveBuffer();

    assert(buffer.Begin != buffer.End);
    const ssize_t readSize = read(UartFd, buffer.Begin, BytesSpanSize(&buffer));
    if (readSize == -1) {
        Exit_DoExitWithLog(ExitCode_UartEvent_Read, "ERROR: read() - %s (%d)\n", strerror(errno), errno);
        return;
    }

    if (readSize > 0) {
        Log_Debug("Received %d bytes.\n", readSize);
        MessageParserAddReceivedSize((size_t)readSize);
        MessageParserDoWork();
    }
}

////////////////////////////////////////////////////////////////////////////////
// Init & Close

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

    MessageParserSetMessageReceivedHandler(MessageReceivedHandler);

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
    EvRegUart = EventLoop_RegisterIo(EvLoop, UartFd, EventLoop_Input, UartReceiveHandler, NULL);
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

////////////////////////////////////////////////////////////////////////////////
// main

int main(int argc, char* argv[])
{
    Log_Debug("Application starting.\n");
    InitPeripheralsAndHandlers();

    Log_Debug("Wait for connect to internet.\n");
    while (true) {
        Networking_InterfaceConnectionStatus status;
        if (Networking_GetInterfaceConnectionStatus("wlan0", &status) == 0) {
            if (status & Networking_InterfaceConnectionStatus_ConnectedToInternet) break;
        }
    }
    Log_Debug("Internet is connected.\n");

    DeviceClient = AzureDeviceClientNew();
    assert(DeviceClient != NULL);
    bool ret = AzureDeviceClientConnectIoTHubUsingDAA(DeviceClient, IoTHubHostName, DeviceId);
    assert(ret);

    while (!Exit_IsExit()) {
        const EventLoop_Run_Result result = EventLoop_Run(EvLoop, -1, true);
        if (result == EventLoop_Run_Failed && errno != EINTR) Exit_DoExit(ExitCode_Main_EventLoopFail);
        AzureDeviceClientDoWork(DeviceClient);
    }

    AzureDeviceClientDelete(DeviceClient);

    Log_Debug("Application exiting.\n");
    ClosePeripheralsAndHandlers();

    const int exitCode = Exit_GetExitCode();
    Log_Debug("Exit Code is %d.\n", exitCode);

    return exitCode;
}

////////////////////////////////////////////////////////////////////////////////
