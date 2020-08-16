// Copyright (c) 2020 matsujirushi
// Released under the MIT license
// https://github.com/matsujirushi/uart-to-azureiot/blob/master/LICENSE.txt

#pragma once

#include <stdbool.h>
#include <azureiot/iothub_device_client_ll.h>
#include "parson.h"

typedef struct {
	IOTHUB_DEVICE_CLIENT_LL_HANDLE Handle;
	bool Connected;
} AzureDeviceClient_t;

AzureDeviceClient_t* AzureDeviceClientNew(void);
void AzureDeviceClientDelete(AzureDeviceClient_t* context);

bool AzureDeviceClientIsConnected(const AzureDeviceClient_t* context);

void AzureDeviceClientDoWork(AzureDeviceClient_t* context);

bool AzureDeviceClientConnectIoTHubUsingDAA(AzureDeviceClient_t* context, const char* iotHubHostName, const char* deviceId);
void AzureDeviceClientDisconnect(AzureDeviceClient_t* context);

bool AzureDeviceClientSendTelemetryAsync(AzureDeviceClient_t* context, const char* messageString);
