// Copyright (c) 2020 matsujirushi
// Released under the MIT license
// https://github.com/matsujirushi/uart-to-azureiot/blob/master/LICENSE.txt

#include "AzureDeviceClient.h"
#include <assert.h>
#include <stdlib.h>
#include <azureiot/iothubtransportmqtt.h>
#include <azure_prov_client/iothub_security_factory.h>

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

bool AzureDeviceClientSendTelemetryAsync(AzureDeviceClient_t* context, JSON_Object* telemetryObject)
{
	assert(context != NULL);
	assert(telemetryObject != NULL);

	char* jsonString = json_serialize_to_string(json_object_get_wrapping_value(telemetryObject));
	IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromString(jsonString);
	json_free_serialized_string(jsonString);
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
