﻿// Copyright (c) 2020 matsujirushi
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
#include <getopt.h>
#include <ctype.h>
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
    ExitCode_Success                    = 0,
    ExitCode_SigTerm                    = 1,

    ExitCode_Validate_ConnectionType    = 2,
    ExitCode_Validate_ScopeId           = 3,
    ExitCode_Validate_IotHubHostname    = 4,
    ExitCode_Validate_DeviceId          = 5,

    ExitCode_Init_EventLoop             = 6,
    ExitCode_Init_ButtonPollTimer       = 7,
    ExitCode_Init_RegisterIo            = 8,
    ExitCode_Init_AzureTimer            = 9,

    ExitCode_Main_EventLoopFail         = 10,

    ExitCode_ButtonTimer_Consume        = 11,
    ExitCode_AzureTimer_Consume         = 12,
    ExitCode_SendMessage_Write          = 13,
    ExitCode_UartEvent_Read             = 14,
} ExitCode;

typedef enum {
    ConnectionType_NotDefined   = 0,
    ConnectionType_DPS          = 1,
    ConnectionType_Direct       = 2
} ConnectionType_t;

typedef enum {
    NetworkState_NoInternet,
    NetworkState_ConnectingAzureIoT,
    NetworkState_ConnectedAzureIoT,
} NetworkState_t;

static int NetworkingLedGreen = -1; // fd
static int NetworkingLedBlue = -1;  // fd

static int ButtonFd = -1;           // fd
static bool ButtonValue = false;

static int UartFd = -1;             // fd

static ConnectionType_t ConnectionType = ConnectionType_NotDefined; // Type of connection to use.
static char* ScopeId = NULL;
static char* IoTHubHostName = NULL;
static char* DeviceId = NULL;
static NetworkState_t NetworkState = NetworkState_NoInternet;
static const char* NetworkInterface = "wlan0";
static AzureDeviceClient_t* DeviceClient = NULL;

static EventLoop* EvLoop = NULL;
static EventLoopTimer* EvTimerButton = NULL;
static EventRegistration* EvRegUart = NULL;
static EventLoopTimer* EvAzureTimer = NULL;

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
        if (bytesSent < 0) {
            Exit_DoExitWithLog(ExitCode_SendMessage_Write, "ERROR: write() - %s (%d)\n", strerror(errno), errno);
            return;
        }

        totalBytesSent += (size_t)bytesSent;
    }

    Log_Debug("Sent %zu bytes in %d calls.\n", totalBytesSent, sendIterations);
}

static void ButtonTimerEventHandler(EventLoopTimer* timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        Exit_DoExit(ExitCode_ButtonTimer_Consume);
        return;
    }

    const bool newButtonValue = GpioReadInv(ButtonFd);
    if (!ButtonValue && newButtonValue) UartSend(UartFd, "label1:11,label2:22\n");
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
    Log_Debug("Received message: \"");
    for (uint8_t* p = messageSpan.Begin; p != messageSpan.End; ++p) Log_Debug("%c", *p);
    Log_Debug("\"\n");


    const int partNumberInMessage = SpNumberOfParts(messageSpan);
    SpPart_t parts[partNumberInMessage];
    int partNumber = 0;
    for (int i = 0; i < partNumberInMessage; ++i) {
        BytesSpan_t partSpan;
        messageSpan = SpGetPartSpan(messageSpan, &partSpan);
        SpPart_t part = SpPartInit(partSpan);
        if (BytesSpanSize(&part.Label) >= 1 && BytesSpanSize(&part.Value) >= 1) {
            parts[partNumber++] = part;
        }
    }

    if (partNumber <= 0) return;

    size_t messageLength = 2;   // {}
    for (int i = 0; i < partNumber; ++i) {
        messageLength += 1 + BytesSpanSize(&parts[i].Label) + 2 + BytesSpanSize(&parts[i].Value) + 1;   // "<Label>":<Value>,
    }
    messageLength -= 1; // ,

    char message[messageLength + 1];
    char* messagePtr = message;
    *messagePtr++ = '{';
    for (int i = 0; i < partNumber; ++i) {
        *messagePtr++ = '\"';
        memcpy(messagePtr, parts[i].Label.Begin, BytesSpanSize(&parts[i].Label));
        messagePtr += BytesSpanSize(&parts[i].Label);
        *messagePtr++ = '\"';
        *messagePtr++ = ':';
        memcpy(messagePtr, parts[i].Value.Begin, BytesSpanSize(&parts[i].Value));
        messagePtr += BytesSpanSize(&parts[i].Value);
        if (i < partNumber - 1) {
            *messagePtr++ = ',';
        }
    }
    *messagePtr++ = '}';
    *messagePtr = '\0';

    Log_Debug("Telemetry message: \"%s\"\n", message);
    if (NetworkState == NetworkState_ConnectedAzureIoT) {
        bool ret = AzureDeviceClientSendTelemetryAsync(DeviceClient, message);
        assert(ret);
    }
    else {
        Log_Debug("Cannot send telemetry message.\n");
    }
}

static void UartReceiveHandler(EventLoop* el, int fd, EventLoop_IoEvents events, void* context)
{
    BytesSpan_t buffer = MessageParserGetReceiveBuffer();

    assert(buffer.Begin != buffer.End);
    const ssize_t readSize = read(UartFd, buffer.Begin, BytesSpanSize(&buffer));
    if (readSize < 0) {
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
// Azure IoT

static bool IsInternetConnected(void)
{
    Networking_InterfaceConnectionStatus status;
    if (Networking_GetInterfaceConnectionStatus(NetworkInterface, &status) != 0) return false;
    if (!(status & Networking_InterfaceConnectionStatus_ConnectedToInternet)) return false;

    return true;
}

static void ConnectAzureIoT(void)
{
    assert(DeviceClient == NULL);
    DeviceClient = AzureDeviceClientNew();
    assert(DeviceClient != NULL);
    bool ret;
    switch (ConnectionType) {
    case ConnectionType_Direct:
        ret = AzureDeviceClientConnectIoTHubUsingDAA(DeviceClient, IoTHubHostName, DeviceId);
        break;
    case ConnectionType_DPS:
        ret = AzureDeviceClientConnectIoTHubUsingDPS(DeviceClient, ScopeId);
        break;
    default:
        abort();
    }
    assert(ret);
}

static void DisconnectAzureIoT(void)
{
    AzureDeviceClientDisconnect(DeviceClient);
    AzureDeviceClientDelete(DeviceClient);
    DeviceClient = NULL;
}

static void AzureTimerEventHandler(EventLoopTimer* timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        Exit_DoExit(ExitCode_AzureTimer_Consume);
    }

    if (DeviceClient != NULL) AzureDeviceClientDoWork(DeviceClient);

    switch (NetworkState) {
    case NetworkState_NoInternet:
        if (IsInternetConnected())
        {
            ConnectAzureIoT();
            NetworkState = NetworkState_ConnectingAzureIoT;
        }
        break;
    case NetworkState_ConnectingAzureIoT:
        if (!IsInternetConnected())
        {
            DisconnectAzureIoT();
            NetworkState = NetworkState_NoInternet;
        }
        if (AzureDeviceClientIsConnected(DeviceClient))
        {
            NetworkState = NetworkState_ConnectedAzureIoT;
        }
        break;
    case NetworkState_ConnectedAzureIoT:
        if (!IsInternetConnected() || !AzureDeviceClientIsConnected(DeviceClient))
        {
            DisconnectAzureIoT();
            NetworkState = NetworkState_NoInternet;
        }
        break;
    default:
        abort();
    }

    switch (NetworkState) {
    case NetworkState_NoInternet:
        GpioWriteInv(NetworkingLedGreen, false);
        GpioWriteInv(NetworkingLedBlue, false);
        break;
    case NetworkState_ConnectingAzureIoT:
        GpioWriteInv(NetworkingLedGreen, false);
        GpioWriteInv(NetworkingLedBlue, true);
        break;
    case NetworkState_ConnectedAzureIoT:
        GpioWriteInv(NetworkingLedGreen, true);
        GpioWriteInv(NetworkingLedBlue, false);
        break;
    default:
        abort();
    }
}

////////////////////////////////////////////////////////////////////////////////
// Init & Close

static void InitPeripheralsAndHandlers(void)
{
    Termination_SetExitCode(ExitCode_SigTerm);

    NetworkingLedGreen = GPIO_OpenAsOutput(NETWORKING_LED_GREEN, GPIO_OutputMode_PushPull, GPIO_Value_High);
    assert(NetworkingLedGreen >= 0);
    NetworkingLedBlue = GPIO_OpenAsOutput(NETWORKING_LED_BLUE, GPIO_OutputMode_PushPull, GPIO_Value_High);
    assert(NetworkingLedBlue >= 0);

    ButtonFd = GPIO_OpenAsInput(SEND_BUTTON);
    assert(ButtonFd >= 0);

    UART_Config uartConfig;
    UART_InitConfig(&uartConfig);
    uartConfig.baudRate = 115200;
    uartConfig.flowControl = UART_FlowControl_None;
    UartFd = UART_Open(ARDUINO_UART, &uartConfig);
    assert(UartFd >= 0);

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
    const struct timespec azurePeriod = { .tv_sec = 1, .tv_nsec = 0 };
    EvAzureTimer = CreateEventLoopPeriodicTimer(EvLoop, &AzureTimerEventHandler, &azurePeriod);
    if (EvAzureTimer == NULL) {
        Exit_DoExitWithLog(ExitCode_Init_AzureTimer, "ERROR: CreateEventLoopPeriodicTimer() - %s (%d)\n", strerror(errno), errno);
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

static void ParseCommandLineArguments(int argc, char* argv[])
{
    int option = 0;
    static const struct option cmdLineOptions[] = {
        { "ConnectionType", required_argument, NULL, 'c'},
        { "ScopeID"       , required_argument, NULL, 's'},
        { "Hostname"      , required_argument, NULL, 'h'},
        { "DeviceID"      , required_argument, NULL, 'd'},
        { NULL            , 0                , NULL, 0  }
    };

    while ((option = getopt_long(argc, argv, "c:s:h:d:", cmdLineOptions, NULL)) != -1) {
        if (optarg != NULL && optarg[0] == '-') {
            Log_Debug("Warning: Option %c requires an argument\n", option);
            continue;
        }
        switch (option) {
        case 'c':
            Log_Debug("ConnectionType: %s\n", optarg);
            if (strcmp(optarg, "DPS") == 0) {
                ConnectionType = ConnectionType_DPS;
            }
            else if (strcmp(optarg, "Direct") == 0) {
                ConnectionType = ConnectionType_Direct;
            }
            break;
        case 's':
            Log_Debug("ScopeID: %s\n", optarg);
            ScopeId = optarg;
            break;
        case 'h':
            Log_Debug("Hostname: %s\n", optarg);
            IoTHubHostName = optarg;
            break;
        case 'd':
            Log_Debug("DeviceID: %s\n", optarg);
            DeviceId = optarg;
            break;
        default:
            break;
        }
    }
}

static ExitCode ValidateUserConfiguration(void)
{
    ExitCode validationExitCode = ExitCode_Success;

    switch (ConnectionType) {
    case ConnectionType_DPS:
        if (ScopeId == NULL) {
            validationExitCode = ExitCode_Validate_ScopeId;
        }
        else {
            Log_Debug("Using DPS Connection: Azure IoT DPS Scope ID %s\n", ScopeId);
        }
        break;
    case ConnectionType_Direct:
        if (IoTHubHostName == NULL) {
            validationExitCode = ExitCode_Validate_IotHubHostname;
        }
        else if (DeviceId == NULL) {
            validationExitCode = ExitCode_Validate_DeviceId;
        }
        if (DeviceId != NULL) {
            // Validate that device ID is in lowercase.
            size_t len = strlen(DeviceId);
            for (size_t i = 0; i < len; i++) {
                if (isupper(DeviceId[i])) {
                    Log_Debug("Device ID must be in lowercase.\n");
                    return ExitCode_Validate_DeviceId;
                }
            }
        }
        if (validationExitCode == ExitCode_Success) {
            Log_Debug("Using Direct Connection: Azure IoT Hub Hostname %s\n", IoTHubHostName);
        }
        break;
    default:
        validationExitCode = ExitCode_Validate_ConnectionType;
        break;
    }

    return validationExitCode;
}

int main(int argc, char* argv[])
{
    Log_Debug("Application starting.\n");

    ParseCommandLineArguments(argc, argv);
    int exitCode = ValidateUserConfiguration();
    if (exitCode != ExitCode_Success) {
        Exit_DoExit(exitCode);
    }
    else {
        InitPeripheralsAndHandlers();

        while (!Exit_IsExit()) {
            const EventLoop_Run_Result result = EventLoop_Run(EvLoop, -1, true);
            if (result == EventLoop_Run_Failed && errno != EINTR) Exit_DoExit(ExitCode_Main_EventLoopFail);
        }

        Log_Debug("Application exiting.\n");
        if (DeviceClient != NULL) DisconnectAzureIoT();
        ClosePeripheralsAndHandlers();
    }

    exitCode = Exit_GetExitCode();
    Log_Debug("Exit Code is %d.\n", exitCode);

    return exitCode;
}

////////////////////////////////////////////////////////////////////////////////
