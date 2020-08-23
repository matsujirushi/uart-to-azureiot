// Copyright (c) 2020 matsujirushi
// Released under the MIT license
// https://github.com/matsujirushi/uart-to-azureiot/blob/master/LICENSE.txt

#include "AzureDeviceClient.h"
#include <assert.h>
#include <stdlib.h>
#include <azureiot/iothubtransportmqtt.h>
#include <azureiot/azure_sphere_provisioning.h>
#include <azure_prov_client/iothub_security_factory.h>

static void AzureDeviceClientConnectionStateCallback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* ctx);

AzureDeviceClient_t* AzureDeviceClientNew(void)
{
	AzureDeviceClient_t* context = (AzureDeviceClient_t*)malloc(sizeof(AzureDeviceClient_t));
	if (context == NULL) return NULL;

	context->Handle = NULL;
	context->Connected = false;

	return context;
}

void AzureDeviceClientDelete(AzureDeviceClient_t* context)
{
	assert(context != NULL);

	AzureDeviceClientDisconnect(context);
	free(context);
}

bool AzureDeviceClientIsConnected(const AzureDeviceClient_t* context)
{
	assert(context != NULL);

	return context->Connected;
}

void AzureDeviceClientDoWork(AzureDeviceClient_t* context)
{
	assert(context != NULL);

	if (context->Handle != NULL) IoTHubDeviceClient_LL_DoWork(context->Handle);
}

bool AzureDeviceClientConnectIoTHubUsingDAA(AzureDeviceClient_t* context, const char* iotHubHostName, const char* deviceId)
{
	assert(context != NULL);

	if (context->Handle != NULL) return false;

	const int result = iothub_security_init(IOTHUB_SECURITY_TYPE_X509);
	if (result != 0) return false;

	context->Handle = IoTHubDeviceClient_LL_CreateFromDeviceAuth(iotHubHostName, deviceId, MQTT_Protocol);
	if (context->Handle == NULL) return false;

	const int deviceIdForDaaCertUsage = 1; // A constant used to direct the IoT SDK to use the DAA cert under the hood.
	if (IoTHubDeviceClient_LL_SetOption(context->Handle, "SetDeviceId", &deviceIdForDaaCertUsage) != IOTHUB_CLIENT_OK) return false;

	if (IoTHubDeviceClient_LL_SetConnectionStatusCallback(context->Handle, AzureDeviceClientConnectionStateCallback, context) != IOTHUB_CLIENT_OK) return false;

	return true;
}

bool AzureDeviceClientConnectIoTHubUsingDPS(AzureDeviceClient_t* context, const char* scopeId)
{
	assert(context != NULL);

	if (context->Handle != NULL) return false;

	IOTHUB_DEVICE_CLIENT_LL_HANDLE handle;
	AZURE_SPHERE_PROV_RETURN_VALUE provResult = IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning(scopeId, 10000, &handle);
	if (provResult.result != AZURE_SPHERE_PROV_RESULT_OK) return false;

	context->Handle = handle;
	if (IoTHubDeviceClient_LL_SetConnectionStatusCallback(context->Handle, AzureDeviceClientConnectionStateCallback, context) != IOTHUB_CLIENT_OK) return false;

	return true;
}


void AzureDeviceClientDisconnect(AzureDeviceClient_t* context)
{
	assert(context != NULL);

	if (context->Handle != NULL)
	{
		IoTHubDeviceClient_LL_Destroy(context->Handle);
		context->Handle = NULL;
		context->Connected = false;
	}
}

bool AzureDeviceClientSendTelemetryAsync(AzureDeviceClient_t* context, const char* messageString)
{
	assert(context != NULL);
	assert(messageString != NULL);

	IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromString(messageString);
	if (messageHandle == NULL) return false;

	bool ret = false;

	if (IoTHubMessage_SetContentTypeSystemProperty(messageHandle, "application%2fjson") != IOTHUB_MESSAGE_OK) goto bye;
	if (IoTHubMessage_SetContentEncodingSystemProperty(messageHandle, "utf-8") != IOTHUB_MESSAGE_OK) goto bye;

	if (IoTHubDeviceClient_LL_SendEventAsync(context->Handle, messageHandle, NULL, NULL) != IOTHUB_CLIENT_OK) goto bye;

	IoTHubMessage_Destroy(messageHandle);

	ret = true;

bye:
	return ret;
}

static void AzureDeviceClientConnectionStateCallback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* ctx)
{
	AzureDeviceClient_t* context = (AzureDeviceClient_t*)ctx;

	context->Connected = result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED;
}
