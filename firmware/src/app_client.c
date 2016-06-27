/***********************************************************************************************************************
 Copyright (c) 2016, Imagination Technologies Limited and/or its affiliated group companies.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 following conditions are met:
     1. Redistributions of source code must retain the above copyright notice, this list of conditions and the
        following disclaimer.
     2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
        following disclaimer in the documentation and/or other materials provided with the distribution.
     3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
        products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE 
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***********************************************************************************************************************/

/*! \file lwm2m_client.c
 *  \brief Awa static client - supports LWM2M resources in Wifire after boot strap to a management server
 */

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "creator/core/creator_threading.h"
#include "creator/core/creator_debug.h"
#include "creator/creator_console.h"
#include "awa\static.h"

#include "config_store.h"
#include "app_config.h"
#include "ui_control.h"
#include "app_client.h"

static CreatorThread _ClientThread;
static AwaStaticClient * _AwaClient;
static bool _Terminate;

#define LOCAL_PORT  6000

#define DEVICE_INSTANCES 1
#define LED_INSTANCES 4

#define LABEL_SIZE 64

#define IPSO_LIGHT_CONTROL_ON_OFF 5850

typedef struct      // urn:oma:lwm2m:oma:3
{
    char Manufacturer[LABEL_SIZE];
    char SerialNumber[LABEL_SIZE];
    // TODO - Reboot Execute
    char DeviceType[LABEL_SIZE];
    char SoftwareVersion[LABEL_SIZE];
} DeviceObject;

static DeviceObject device[DEVICE_INSTANCES];
static LEDObject leds[LED_INSTANCES];
static LEDObject prevLeds[LED_INSTANCES];

static void ClientProcess(CreatorThread thread, void *context);
static void ClientSetup(void);
static void CreateDevice(AwaStaticClient * awaClient);
static void CreateLeds(AwaStaticClient * awaClient);

void Client_Initialise(void)
{
    _Terminate = false;

    Creator_Log(CreatorLogLevel_Info, "Client init started");

    _AwaClient = AwaStaticClient_New();
    Client_SetLogLevel(ConfigStore_GetLoggingLevel());

    AwaStaticClient_SetEndPointName(_AwaClient, ConfigStore_GetDeviceName());
    AwaStaticClient_SetBootstrapServerURI(_AwaClient, ConfigStore_GetBootstrapURL());
    AwaStaticClient_SetCoAPListenAddressPort(_AwaClient, "0.0.0.0", LOCAL_PORT);
    AwaStaticClient_Init(_AwaClient);

    CreateDevice(_AwaClient);
    CreateLeds(_AwaClient);
    Creator_Log(CreatorLogLevel_Info, "Client init done");

    _ClientThread = CreatorThread_New("Lwm2mClient", 1, 4096, ClientProcess, NULL);
}

void Client_Shutdown(void)
{
    _Terminate = true;
    if (_ClientThread)
    {
        CreatorThread_Join(_ClientThread);     // Beware join ignored for FreeRTOS. Free too soon could assert before reset
        CreatorThread_Free(&_ClientThread);
    }
}

void UpdateLEDs(void)
{
    int index;
    for (index = 0; index < MAX_LEDS; index++)
    {
        if (prevLeds[index].OnOff != leds[index].OnOff)
        {
            prevLeds[index].OnOff = leds[index].OnOff;
            UIControl_SetLEDMode(index + 1, UILEDMode_Manual);
            if (prevLeds[index].OnOff)
                UIControl_SetLEDState(index + 1, UILEDState_On);
            else
                UIControl_SetLEDState(index + 1, UILEDState_Off);
            Creator_Log(CreatorLogLevel_Info, "Set LED%d %s", index + 1, prevLeds[index].OnOff ? "On" : "Off");
        }
    }
}

static void ClientProcess(CreatorThread thread, void *context)
{
    while (!_Terminate)
    {
        AwaStaticClient_Process(_AwaClient);
        UpdateLEDs();           // TODO - remove
        CreatorThread_SleepMilliseconds(NULL, 1);
    }
    // TODO - un-register (if registered) and wait for sent (with t/o) - or do in other layer?
    Creator_Log(CreatorLogLevel_Warning, "Client closed");
}

static void CreateDevice(AwaStaticClient * awaClient)
{
    char softwareVersion[LABEL_SIZE];
    char serialNumber[17];
    bool validSerialNumber;

    // Define resources
    AwaStaticClient_DefineObject(awaClient, 3, "Device", 0, DEVICE_INSTANCES);
    AwaStaticClient_DefineResource(awaClient, 3, 0, "Manufacturer", AwaResourceType_String, 0, 1, AwaResourceOperations_ReadOnly);
    AwaStaticClient_SetResourceStorageWithPointer(awaClient, 3, 0, &device[0].Manufacturer, sizeof(device[0].Manufacturer), sizeof(device[0]));
    AwaStaticClient_DefineResource(awaClient, 3, 2, "SerialNumber", AwaResourceType_String, 0, 1, AwaResourceOperations_ReadOnly);
    AwaStaticClient_SetResourceStorageWithPointer(awaClient, 3, 2, &device[0].SerialNumber, sizeof(device[0].SerialNumber), sizeof(device[0]));
    AwaStaticClient_DefineResource(awaClient, 3, 17, "DeviceType", AwaResourceType_String, 0, 1, AwaResourceOperations_ReadOnly);
    AwaStaticClient_SetResourceStorageWithPointer(awaClient, 3, 17, &device[0].DeviceType, sizeof(device[0].DeviceType), sizeof(device[0]));
    AwaStaticClient_DefineResource(awaClient, 3, 19, "SoftwareVersion", AwaResourceType_String, 0, 1, AwaResourceOperations_ReadOnly);
    AwaStaticClient_SetResourceStorageWithPointer(awaClient, 3, 19, &device[0].SoftwareVersion, sizeof(device[0].SoftwareVersion), sizeof(device[0]));

    // Get initial values
    AppInfo *appInfo = AppConfig_GetAppInfo();
    snprintf(softwareVersion, LABEL_SIZE, "%s (%s)", appInfo->ApplicationVersion, appInfo->ApplicationVersionDate);
    memset(serialNumber, 0, sizeof(serialNumber));
    validSerialNumber = DeviceSerial_GetCpuSerialNumberHexString(serialNumber, 17);

    // Set initial values
    int instance = 0;
    AwaStaticClient_CreateObjectInstance(awaClient, 3, instance);
    AwaStaticClient_CreateResource(awaClient, 3, instance, 0);
    strncpy(device[instance].Manufacturer, "Imagination Technologies Ltd", LABEL_SIZE);
    if (validSerialNumber)
    {
        AwaStaticClient_CreateResource(awaClient, 3, instance, 2);
        strncpy(device[instance].SerialNumber, serialNumber, LABEL_SIZE);
    }
    AwaStaticClient_CreateResource(awaClient, 3, instance, 17);
    strncpy(device[instance].DeviceType, ConfigStore_GetDeviceType(), LABEL_SIZE);
    AwaStaticClient_CreateResource(awaClient, 3, instance, 19);
    strncpy(device[instance].SoftwareVersion, softwareVersion, LABEL_SIZE);
}

static void CreateLeds(AwaStaticClient * awaClient)
{
    // Define resources
    // TODO - use callback API instead - to support live read/writes
    AwaStaticClient_DefineObject(awaClient, 3311, "Light", 0, LED_INSTANCES);
    AwaStaticClient_DefineResource(awaClient, 3311, IPSO_LIGHT_CONTROL_ON_OFF, "OnOff", AwaResourceType_Boolean, 0, 1, AwaResourceOperations_ReadWrite);
    AwaStaticClient_SetResourceStorageWithPointer(awaClient, 3311, IPSO_LIGHT_CONTROL_ON_OFF, &leds[0].OnOff, sizeof(leds[0].OnOff), sizeof(leds[0]));

    // Set initial values
    int instance;
    for (instance = 0; instance < LED_INSTANCES; instance++)
    {
        AwaStaticClient_CreateObjectInstance(awaClient, 3311, instance);
        AwaStaticClient_CreateResource(awaClient, 3311, instance, IPSO_LIGHT_CONTROL_ON_OFF);
        leds[instance].OnOff = true;

        prevLeds[instance].OnOff = true;
    }
}

LEDObject * Client_GetLeds(void)
{
    return leds;
}

void Client_SetLogLevel(CreatorLogLevel level)
{
    switch (level) {
        case CreatorLogLevel_Error:
            AwaStaticClient_SetLogLevel(AwaLogLevel_Error);
            break;
        case CreatorLogLevel_Warning:
            AwaStaticClient_SetLogLevel(AwaLogLevel_Warning);
            break;
        case CreatorLogLevel_Info:
            AwaStaticClient_SetLogLevel(AwaLogLevel_Verbose);
            break;
        case CreatorLogLevel_Debug:
            AwaStaticClient_SetLogLevel(AwaLogLevel_Debug);
            break;
        default:
            CreatorConsole_Printf("Unknown LogLevel: %d\r\n", level);
            break;
    }
}
