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

#include <string.h>
#include "app.h"
#include "command_handlers.h"

#include "creator_command.h"
#include "config_store.h"
#include "device_serial.h"
#include "string_builder.h"
#include "creator/creator_console.h"


typedef struct
{
    int temperatureReading_whole;
    int temperatureReading_fractional;
    int humidityReading_whole;
    int humidityReading_fractional;
} AppDataStoreItem;

typedef enum
{
    app_setshow_cmd_owned_devices,      // TODO - replace app commands
    app_setshow_cmd_saved_values,
    app_setshow_cmd__max
} app_SetShowCommand;

typedef struct
{
    const char* Name;
    bool IsSetCommand;
    bool IsShowCommand;
} setShowCommandInfo;

static setShowCommandInfo app_SetShowCommands[app_setshow_cmd__max] =
{      //   Name            Set?    Show?
        { "owned_devices", false, true },
        { "saved_values", false, true } };

static bool CommandBoardDetails(int argc, char** argv);			// command board_details
static bool CommandClearSavedValues(int argc, char** argv);		// command clear_saved_values
static bool CommandSaveValue(int argc, char** argv);				// command save_value
static bool CommandUpdateOwnedDevices(int argc, char** argv);		// command update_owned_devices
static bool CommandSendPublishCommand(int argc, char** argv);	// command send_message
static bool CommandRemoveOwnership(int argc, char** argv);			// command delete device

#define STATERAPP_COMMAND_GROUP "app_commands"
#define LINE_TERM "\r\n"          // line terminator

void AppCommands_Initialise(void)
{
    // TODO - Application specific commands
    CreatorCommand_RegisterCommandGroup(STATERAPP_COMMAND_GROUP, ": Application Commands");
    CreatorCommand_RegisterCommand(STATERAPP_COMMAND_GROUP, "board_details", "Display board information for output to label printing software",
            CommandBoardDetails);
//	CreatorCommand_RegisterCommand(STATERAPP_COMMAND_GROUP, "send_command", "Publish command to this device's owner, or a peer device", CommandSendPublishCommand);
}

bool AppCommands_CommandShow(int argc, char** argv)
{
    bool result = true;
//	if (argc == 2)
//	{
//		if (argv[1])
//		{
//			if (strcmp(argv[1], app_SetShowCommands[app_setshow_cmd_owned_devices].Name) == 0)
//			{
//         		CreatorConsole_Puts("Owned devices - not supported" LINE_TERM);        // TODO - delete/update Creator commands
//			}
//			else if (strcmp(argv[1], app_SetShowCommands[app_setshow_cmd_saved_values].Name) == 0)
//			{
//         		CreatorConsole_Puts("Measurement commands - not supported" LINE_TERM);
//			}
//			else
//			{
//				result = false;
//			}
//		}
//		else
//		{
//			result = false;
//		}
//	}
//	else
//	{
//		if (argc > 2)
//			result = false;
//
//        // TODO - delete?
//		CreatorConsole_Puts("Starter App Supported:" LINE_TERM);
//		int index;
//		for (index = 0; index < app_setshow_cmd__max; index++)
//		{
//            if (app_SetShowCommands[index].IsShowCommand)
//            {                
//                CreatorConsole_Puts("\t\t");
//                CreatorConsole_Puts(app_SetShowCommands[index].Name);
//                CreatorConsole_Puts(LINE_TERM);
//            }
//		}
//	}
    return result;
}

static bool CommandBoardDetails(int argc, char** argv)
{
    bool result = false;
    if (ConfigStore_Config_Read() && ConfigStore_Config_IsValid() && ConfigStore_Config_IsMagicValid())
    {
        StringBuilder response = StringBuilder_New(256);

        // DeviceType
        response = StringBuilder_Append(response, ConfigStore_GetDeviceType());
        response = StringBuilder_Append(response, LINE_TERM);

        // MAC address
        response = StringBuilder_Append(response, ConfigStore_GetMacAddress());
        response = StringBuilder_Append(response, LINE_TERM);

        // Serial number
        char snString[17];
        memset((void*) snString, 0, 17);
        if (DeviceSerial_GetCpuSerialNumberHexString(snString, 17))
            response = StringBuilder_Append(response, snString);
        else
            response = StringBuilder_Append(response, " ");
        response = StringBuilder_Append(response, LINE_TERM);
        // WiFi SoftAP SSID
        response = StringBuilder_Append(response, ConfigStore_GetSoftAPSSID());
        response = StringBuilder_Append(response, LINE_TERM);

        // WiFi SoftAP password
        response = StringBuilder_Append(response, ConfigStore_GetSoftAPPassword());
        response = StringBuilder_Append(response, LINE_TERM);
        char CB[2] =
        { 0, 0 };
        int32_t byteCount = StringBuilder_GetLength(response);
        int32_t byteIndex = 0;
        for (byteIndex = 0; byteIndex < byteCount; byteIndex++)
        {
            CB[0] += (StringBuilder_GetCString(response))[byteIndex];
        }
        // Compute checkbyte - 2's compliment of MOD-256 sum
        CB[0] ^= 0xFF;
        CB[0]++;
        response = StringBuilder_Append(response, CB);
        response = StringBuilder_Append(response, LINE_TERM);
        CreatorConsole_Puts(response);

        // Output entire response  // TODO - delete?
//		byteCount = StringBuilder_GetLength(response);
//		uint8_t* _response = (uint8_t*) StringBuilder_GetCString(response);
//		for (byteIndex = 0; byteIndex < byteCount; byteIndex++)
//		{
//			if (((char *) _response)[byteIndex] == '\n')
//				CreatorConsole_Puts(LINE_TERM);
//			else
//				CreatorConsole_Putc(((char *) _response)[byteIndex]);
//		}
        //CreatorConsole_Puts(LINE_TERM);

        StringBuilder_Free(&response);
        result = true;
    }
    return result;
}


