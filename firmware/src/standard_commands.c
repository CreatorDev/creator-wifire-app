/***********************************************************************************************************************
 Copyright (c) 2016, Imagination Technologies Limited
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

#include "app_config.h"
#include "config_store.h"
#include "activitylog.h"
#include "creator_command.h"
#include "standard_commands.h"
#include "string_builder.h"

#include "creator/creator_console.h"
#include "creator/core/creator_list.h"

#include "tcpip/tcpip.h"
#include "driver/wifi/mrf24w/drv_wifi.h"
#include "system/debug/sys_debug.h"

//
// Type Definitions
//

// Supported Set/Show command list
typedef enum
{
	setshow_cmd_devicename = 0,
	setshow_cmd_logconfig,
	setshow_cmd_networkconfig,
	setshow_cmd_versions,
	setshow_cmd_wifire_details,
	setshow_cmd_saved_values,

	setshow_cmd__max
} setShowCommand;

typedef void (*SetShowCommandHandler) (void);

typedef struct
{
    const char* Name;
    SetShowCommandHandler SetCallBack;
    SetShowCommandHandler ShowCallBack;
} setShowCommandInfo;

#define LINE_TERM   "\r\n"
#define LINE_TAB    "\t"


//
// Variables
//

extern char g_CommandAlive;
extern bool g_CommandTimeOut;


//
// Function Prototypes
//

static void StandardCommands_GetDeviceServerConfig(void);    
static void StandardCommands_SetDeviceServerConfig(void);
static void StandardCommands_SetNetworkConfig(void);    
static void StandardCommands_ShowWiFireDetails(void);

//                                                                   Name                    SetCallback?                    ShowCallback?
static setShowCommandInfo setShowCommands[setshow_cmd__max] = { {"device_name",    StandardCommands_SetDeviceName,    StandardCommands_GetDeviceName},
                                                                {"log_config",     StandardCommands_SetLogConfig,     StandardCommands_GetLogConfig},
                                                                {"network_config", StandardCommands_SetNetworkConfig, StandardCommands_GetNetworkConfig},
                                                                {"server_config",  StandardCommands_SetDeviceServerConfig, StandardCommands_GetDeviceServerConfig},
                                                                {"versions",       NULL,                              StandardCommands_GetVersions},
                                                                {"wifire_details", NULL,                              StandardCommands_ShowWiFireDetails}
                                                             };


//
// Command Handler Definitions
//

bool StandardCommands_BoardDetails(int argc, char** argv)
{
	bool result = false;
	if(ConfigStore_Config_Read() && ConfigStore_Config_IsValid())
	{
		StringBuilder response = StringBuilder_New(256);
		// DeviceType
		response = StringBuilder_Append(response, ConfigStore_GetDeviceType());
		response = StringBuilder_Append(response, "\n");
		// MAC address
		response = StringBuilder_Append(response, ConfigStore_GetMacAddress());
		response = StringBuilder_Append(response, "\n");
		// Serial number
		char snString[17];
		memset((void*) snString, 0, 17);
		if(DeviceSerial_GetCpuSerialNumberHexString(snString, 17))
			response = StringBuilder_Append(response, snString);
		else
			response = StringBuilder_Append(response, " ");
		response = StringBuilder_Append(response, "\n");

		// WiFi SoftAP SSID
		response = StringBuilder_Append(response, ConfigStore_GetSoftAPSSID());
		response = StringBuilder_Append(response, "\n");

		// WiFi SoftAP password
		response = StringBuilder_Append(response, ConfigStore_GetSoftAPPassword());
		response = StringBuilder_Append(response, "\n");

		char CB[2] = {0,0};
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
		response = StringBuilder_Append(response, "\n");

		// Output entire response
		byteCount = StringBuilder_GetLength(response);
		uint8_t* _response = (uint8_t*) StringBuilder_GetCString(response);
		for (byteIndex = 0; byteIndex < byteCount; byteIndex++)
		{
			CreatorConsole_Putc(((char *) _response)[byteIndex]);
		}
		CreatorConsole_Puts(LINE_TERM);

		StringBuilder_Free(&response);
		result = true;
	}    
	return result;
}

bool StandardCommands_FactoryResetHelper(void)
{
	bool result = false;

	// Reset configuration to default state
	if (ConfigStore_Config_Read() && ConfigStore_LoggingSettings_Read())
	{
		if (ConfigStore_Config_ResetToDefaults() && ConfigStore_LoggingSettings_ResetToDefaults() && ConfigStore_Config_Write() && ConfigStore_LoggingSettings_Write())
		{
			CreatorConsole_Puts("Successfully reset device to factory-default state\n\r");
			result = true;
		}
	}

	if (!result)
		SYS_ERROR(SYS_ERROR_ERROR, "command_handlers: Error, Could not reset device to factory-default settings");

	return result;
}

bool StandardCommands_FactoryReset(int argc, char** argv)
{
	bool result = true;
    // Confirm with user before resetting
    if (CreatorCommand_PromptUserWithQuery("Are you sure you want to reset this device to its factory-default settings? (y/n) "))
    {
        bool preserveSoftAPPassword = CreatorCommand_PromptUserWithQuery("Would you like to preserve your device's current Configuration mode password: (y/n) ");
        char *existingSoftAPPassword = NULL;
        if (preserveSoftAPPassword)
        {
            const char* softApPassword = ConfigStore_GetSoftAPPassword();
            uint8 softApPasswordLength = strlen(softApPassword);

            existingSoftAPPassword = (char*) Creator_MemAlloc((softApPasswordLength + 1) * sizeof(char));
            if (existingSoftAPPassword)
            {
                memset(existingSoftAPPassword, '\0', (softApPasswordLength + 1) * sizeof(char));
                memcpy(existingSoftAPPassword, softApPassword, softApPasswordLength);
            }
        }

        if (!StandardCommands_FactoryResetHelper())
        {
            result = false;
        }
        else
        {
            if (preserveSoftAPPassword && existingSoftAPPassword)
            {
                if (!ConfigStore_SetSoftAPPassword(existingSoftAPPassword) || !ConfigStore_Config_UpdateCheckbyte() || !ConfigStore_Config_Write())
                    result = -1;

                Creator_MemFree((void **) &existingSoftAPPassword);
            }
        }

        AppInfo *appInfo = AppConfig_GetAppInfo();
        if (!AppConfig_IsRunningInConfigurationMode() && appInfo != NULL && appInfo->AppCLI_ResetHandler != NULL)
        {
            result = appInfo->AppCLI_ResetHandler(true);
            ConfigStore_Config_UpdateCheckbyte();
            ConfigStore_Config_Write();
            // TODO - unregister from lwm2m server
            AppConfig_SetDeviceOnline(false);
        }
        else
            AppConfig_SoftwareReset(true);

    }
    else
    {
        CreatorConsole_Printf("Aborted." LINE_TERM);
    }

	return result;
}

bool StandardCommands_Reboot(int argc, char** argv)
{
	int result = true;
    AppInfo *appInfo = AppConfig_GetAppInfo();
    if (!AppConfig_IsRunningInConfigurationMode() && appInfo != NULL && appInfo->AppCLI_ResetHandler != NULL)
    {
        CreatorConsole_Printf("Requesting reboot..." LINE_TERM);
        result = appInfo->AppCLI_ResetHandler(false);
    }
    else
    {
        CreatorConsole_Printf("Rebooting..." LINE_TERM);
        AppConfig_SoftwareReset(false);
    }
	return result;
}

bool StandardCommands_RebootSoftAP(int argc, char** argv)
{
	int result = true;
    AppInfo *appInfo = AppConfig_GetAppInfo();
	if (!AppConfig_IsRunningInConfigurationMode() && appInfo != NULL && appInfo->AppCLI_ResetHandler != NULL)
		result = appInfo->AppCLI_ResetHandler(true);
	else
		AppConfig_SoftwareReset(true);

	return result;
}

bool StandardCommands_Set(int argc, char** argv)
{
	bool result = true;
    if (argc == 2)
    {
        if (argv[1])
        {
            unsigned int index = 0;
            bool commandFound = false;
            for (index = 0; index < setshow_cmd__max; index++)
            {
                if (setShowCommands[index].Name != NULL)
                {
                    if (strcmp(argv[1], setShowCommands[index].Name) == 0)
                    {
                        if (setShowCommands[index].SetCallBack != NULL)
                        {
                            setShowCommands[index].SetCallBack();
                            commandFound = true;
                            break;
                        }
                    }
                }
            }

            if (!commandFound)
            {
                result = false;
                AppInfo *appInfo = AppConfig_GetAppInfo();
                if (!AppConfig_IsRunningInConfigurationMode() && appInfo != NULL && appInfo->CommandSet != NULL)
                    result = appInfo->CommandSet(argc, argv);

                if (!result)
                    CreatorConsole_Printf("Invalid command" LINE_TERM);
            }
        }
        else
        {
            CreatorConsole_Printf("Error null argument. Ignoring command" LINE_TERM);
            result = false;
        }
    }
    else
    {
        if (argc > 2)
            CreatorConsole_Printf("Invalid command" LINE_TERM);

        AppInfo *appInfo = AppConfig_GetAppInfo();
        CreatorConsole_Printf("'set' command usage:\tset <setting_name>" LINE_TERM);
		CreatorConsole_Puts("Supported:\r\n");
		int index;
		for (index = 0; index < setshow_cmd__max; index++)
		{
            if (setShowCommands[index].SetCallBack != NULL)
            {
                CreatorConsole_Puts("\t\t");
                CreatorConsole_Puts(setShowCommands[index].Name);
                CreatorConsole_Puts(LINE_TERM);
            }
		}
        if (!AppConfig_IsRunningInConfigurationMode() && appInfo != NULL && appInfo->CommandSet != NULL)
            result = appInfo->CommandSet(argc, argv);

        result = false;
    }
	return result;
}

bool StandardCommands_Show(int argc, char** argv)
{
	bool result = true;
    if (argc == 2)
    {       
        if (argv[1])
        {
            unsigned int index = 0;
            bool commandFound = false;
            for (index = 0; index < setshow_cmd__max; index++)
            {
                if (setShowCommands[index].Name != NULL)
                {
                    if (strcmp(argv[1], setShowCommands[index].Name) == 0)
                    {
                        if (setShowCommands[index].ShowCallBack != NULL)
                        {
                            setShowCommands[index].ShowCallBack();
                            commandFound = true;
                            break;
                        }
                    }
                }
            }

            if (!commandFound)
            {
                result = false;
                AppInfo *appInfo = AppConfig_GetAppInfo();
                if (!AppConfig_IsRunningInConfigurationMode() && appInfo && appInfo->CommandShow != NULL)
                    result = appInfo->CommandShow(argc, argv);

                if (!result)
                    CreatorConsole_Printf("Invalid command" LINE_TERM);
            }
        }
        else
        {
            CreatorConsole_Printf("Error null argument. Ignoring command" LINE_TERM);
            result = false;
        }
    }
    else
    {
        if (argc > 2)
            CreatorConsole_Printf("Invalid command" LINE_TERM);

        CreatorConsole_Printf("'show' command usage:\tshow <setting_name>" LINE_TERM);
		CreatorConsole_Puts("Supported:" LINE_TERM);
		int index;
		for (index = 0; index < setshow_cmd__max; index++)
		{
            if (setShowCommands[index].ShowCallBack != NULL)
            {
                CreatorConsole_Puts("\t\t");
                CreatorConsole_Puts(setShowCommands[index].Name);
                CreatorConsole_Puts(LINE_TERM);
            }
		}	
        AppInfo *appInfo = AppConfig_GetAppInfo();        
        if (!AppConfig_IsRunningInConfigurationMode() && appInfo != NULL && appInfo->CommandShow != NULL)
            result = appInfo->CommandShow(argc, argv);

        result = false;
    }

	return result;
}

bool StandardCommands_UpdateOwnedDevices(int argc, char** argv)
{
    CommandHandlers_UpdateOwnedDevices();
	return true;
}

bool StandardCommands_Uptime(int argc, char** argv)
{
	bool result = true;
	SYS_UPTIME uptime;
	AppConfig_Uptime(&uptime);
	CreatorConsole_Printf("Device uptime: %dd, %dh, %dm, %ds" LINE_TERM, uptime.Days, uptime.Hours, uptime.Minutes, uptime.Seconds);

	return result;
}

/*Get Command Handlers*/
void StandardCommands_GetVersions(void)
{
		// Application and library information
        AppInfo *appInfo = AppConfig_GetAppInfo();
        if(appInfo != NULL)
            CreatorConsole_Printf("application:\t\t%s\n\r\t\t\tv%s (%s)" LINE_TERM LINE_TERM, appInfo->ApplicationName == NULL ? "no application name" : appInfo->ApplicationName, appInfo->ApplicationVersion == NULL ? "no application version" : appInfo->ApplicationVersion, appInfo->ApplicationVersionDate == NULL ? "no version date" : appInfo->ApplicationVersionDate);
        
		CreatorConsole_Printf("libcreatorcore:\t\tv%s (%s)" LINE_TERM, CreatorCore_GetVersion(), CreatorCore_GetVersionDate());

		// Wifi module software version
		DRV_WIFI_DEVICE_INFO deviceInfo;
		DRV_WIFI_DeviceInfoGet(&deviceInfo);
		CreatorConsole_Printf("MRF24W:\t\t\t0x%02X%02X" LINE_TERM, deviceInfo.romVersion, deviceInfo.patchVersion);

		CreatorConsole_Printf(LINE_TERM LINE_TERM);
}

void StandardCommands_GetDeviceName(void)
{
    if (ConfigStore_Config_Read() && ConfigStore_Config_IsValid())
    {
        char *deviceName = (char *) ConfigStore_GetDeviceName();
        if (deviceName && strlen(deviceName) > 0)
            CreatorConsole_Printf("Device's name: %s" LINE_TERM, deviceName);
        else
            CreatorConsole_Printf("Device's name: not set" LINE_TERM);
    }
    else
    {
        CreatorConsole_Printf("Error, device's configuration is invalid. Please reset the configuration using the 'factory_reset' command" LINE_TERM);
    }
}

void StandardCommands_GetLogConfig(void)
{
    if (ConfigStore_LoggingSettings_Read() && ConfigStore_LoggingSettings_IsValid())
    {
        // Logging 'enabled' setting
        bool logEnabled = ConfigStore_GetLoggingEnabled();
        CreatorConsole_Printf("Logging:\t\t");
        if (logEnabled)
            CreatorConsole_Printf("Enabled" LINE_TERM);
        else
            CreatorConsole_Printf("Disabled" LINE_TERM);

        // Logging level
        CreatorActivityLogLevel logLevel = ConfigStore_GetLoggingLevel();
        CreatorConsole_Printf("Log Level:\t\t");
        if (logLevel < CreatorActivityLogLevel_Max)
            CreatorConsole_Printf("%s" LINE_TERM, (char*) ConfigStore_GetLoggingLevelName(logLevel));
        else
            CreatorConsole_Printf("INVALID" LINE_TERM);

        // Logging categories
        CreatorActivityLogCategory logCategories = ConfigStore_GetLoggingCategories();
        CreatorConsole_Printf("Log categories:\t\t");

        if (logCategories != 0)
        {
            if (logCategories & 1 << CreatorActivityLogCategory_HardwareBoot)
                CreatorConsole_Printf("hwboot ");

            if (logCategories & 1 << CreatorActivityLogCategory_Startup)
                CreatorConsole_Printf("startup ");

            if (logCategories & 1 << CreatorActivityLogCategory_SystemRuntime)
                CreatorConsole_Printf("runtime ");

            if (logCategories & 1 << CreatorActivityLogCategory_App)
                CreatorConsole_Printf("app ");

            if (logCategories & 1 << CreatorActivityLogCategory_Shutdown)
                CreatorConsole_Printf("shutdown ");
        }
        else
        {
            CreatorConsole_Printf("<none selected>");
        }
        CreatorConsole_Printf(LINE_TERM LINE_TERM);
    }
    else
    {
        CreatorConsole_Printf("Error, device's configuration is invalid. Please reset the configuration using the 'factory_reset' command" LINE_TERM);
    }
}

void StandardCommands_GetNetworkConfig(void)
{
    if (ConfigStore_Config_Read() && ConfigStore_Config_IsValid())
    {
        // WiFi SSID setting
        char *wifiSSID = (char *) ConfigStore_GetNetworkSSID();
        if (wifiSSID && strlen(wifiSSID))
            CreatorConsole_Printf("WiFi Network SSID:\t\t%s" LINE_TERM, wifiSSID);
        else
            CreatorConsole_Printf("WiFi Network SSID:\t\tnot set" LINE_TERM);

        // WiFi Encryption Type
        WiFiEncryptionType wifiEncryption = ConfigStore_GetEncryptionType();
        CreatorConsole_Printf("WiFi encryption:\t\t");
        switch (wifiEncryption)
        {
            case WiFiEncryptionType_WEP:
                CreatorConsole_Printf("WEP" LINE_TERM);
                break;

            case WiFiEncryptionType_WPA:
                CreatorConsole_Printf("WPA" LINE_TERM);
                break;

            case WiFiEncryptionType_WPA2:
                CreatorConsole_Printf("WPA2" LINE_TERM);
                break;

            case WiFiEncryptionType_Open:
                CreatorConsole_Printf("Open" LINE_TERM);
                break;

            default:
                CreatorConsole_Printf("Invalid setting. Please define an encryption type" LINE_TERM);
                break;
        }

        // WiFi password setting (if not 'Open' encryption)
        if (ConfigStore_GetEncryptionType() != WiFiEncryptionType_Open)
        {
            char *wifiPassword = (char *) ConfigStore_GetNetworkPassword();
            CreatorConsole_Printf("WiFi Network Password:\t\t");
            if (wifiPassword && strlen(wifiPassword))
            {
                unsigned int index = 0;
                for (index = 0 ; index < strlen(wifiPassword); index++)
                {
                    CreatorConsole_Putc('X');
                }
                CreatorConsole_Printf(LINE_TERM);
            }
            else
                CreatorConsole_Printf("not set" LINE_TERM);
        }

        // IP addressing scheme
        AddressScheme addressingScheme = ConfigStore_GetAddressingScheme();
        CreatorConsole_Printf("Addressing scheme:\t\t");
        switch (addressingScheme)
        {
            case AddressScheme_Dhcp:
                CreatorConsole_Printf("DHCP" LINE_TERM);
                break;

            case AddressScheme_StaticIP:
                CreatorConsole_Printf("Static" LINE_TERM);
                break;

            default:
                CreatorConsole_Printf("Invalid setting. Please define an addressing scheme" LINE_TERM);
                break;
        }

        // Static address assignment settings
        if (addressingScheme == AddressScheme_StaticIP)
        {
            // Static IP address setting
            char *staticIP = (char *) ConfigStore_GetStaticIP();
            if (staticIP && strlen(staticIP))
                CreatorConsole_Printf("Static IP:\t\t\t%s" LINE_TERM, staticIP);
            else
                CreatorConsole_Printf("Static IP:\t\t\tnot set" LINE_TERM);

            // Static DNS setting
            char *staticDNS = (char *) ConfigStore_GetStaticDNS();
            if (staticDNS && strlen(staticDNS))
                CreatorConsole_Printf("Static DNS:\t\t\t%s" LINE_TERM, staticDNS);
            else
                CreatorConsole_Printf("Static DNS:\t\t\tnot set" LINE_TERM);

            // Static Gateway
            char *staticGateway = (char *) ConfigStore_GetStaticGateway();
            if (staticGateway && strlen(staticGateway))
                CreatorConsole_Printf("Static Gateway:\t\t\t%s" LINE_TERM, staticGateway);
            else
                CreatorConsole_Printf("Static Gateway:\t\t\tnot set" LINE_TERM);

            // Static Netmask
            char *staticNetmask = (char *) ConfigStore_GetStaticNetmask();
            if (staticNetmask && strlen(staticNetmask))
                CreatorConsole_Printf("Static Netmask:\t\t\t%s" LINE_TERM, staticNetmask);
            else
                CreatorConsole_Printf("Static Netmask:\t\t\tnot set" LINE_TERM);

        }

        // Configuration Mode setting
        bool bootIntoConfigurationMode = ConfigStore_GetStartInConfigurationMode();
        CreatorConsole_Printf("Boot Into Configuration Mode:\t%s" LINE_TERM, bootIntoConfigurationMode ? "True": "False" );
        
        CreatorConsole_Printf(LINE_TERM LINE_TERM);
    }
    else
    {
        CreatorConsole_Printf("Error, device's configuration is invalid. Please reset the configuration using the 'factory_reset' command" LINE_TERM);
    }
}

void StandardCommands_GetDeviceServerConfig(void)
{
    if (ConfigStore_DeviceServerConfig_Read() && ConfigStore_DeviceServerConfig_IsValid())
    {
        char *bootUrl = (char *) ConfigStore_GetBootstrapURL();
        if (bootUrl && strlen(bootUrl))
            CreatorConsole_Printf("Boot server URL:\t%s" LINE_TERM, bootUrl);
        else
            CreatorConsole_Printf("Boot server URL:\tnot set" LINE_TERM);

        ServerSecurityMode securityMode = ConfigStore_GetSecurityMode();
        CreatorConsole_Printf("Security mode:\t\t");
        switch (securityMode)
        {
            case ServerSecurityMode_NoSec:
                CreatorConsole_Printf("NoSec" LINE_TERM);
                break;

            case ServerSecurityMode_PSK:
                CreatorConsole_Printf("PSK" LINE_TERM);
                break;

            case ServerSecurityMode_Cert:
                CreatorConsole_Printf("Cert" LINE_TERM);
                break;

            default:
                CreatorConsole_Printf("Invalid setting" LINE_TERM);
                break;
        }

        if (securityMode == ServerSecurityMode_PSK)
        {
            char *publicKey = (char *) ConfigStore_GetPublicKey();
            if (publicKey && strlen(publicKey))
                CreatorConsole_Printf("Public key:\t\t%s" LINE_TERM, publicKey);
            else
                CreatorConsole_Printf("Public key:\t\tnot set" LINE_TERM);
            
            char *privateKey = (char *) ConfigStore_GetPrivateKey();
            if (privateKey && strlen(privateKey))
                CreatorConsole_Printf("Private key:\t\tlength = %d" LINE_TERM, strlen(privateKey));
            else
                CreatorConsole_Printf("Private key:\t\tnot set" LINE_TERM);
        }
        
        if (securityMode == ServerSecurityMode_Cert)
        {
            char *cert = (char *) ConfigStore_GetCertificate();
            if (cert && strlen(cert))
                CreatorConsole_Printf("Certificate:\t\tlength = %d" LINE_TERM, strlen(cert));
            else
                CreatorConsole_Printf("Certificate:\t\tnot set" LINE_TERM);
            
            char *certChain = (char *) ConfigStore_GetBootstrapCertChain();
            if (certChain && strlen(certChain))
                CreatorConsole_Printf("Bootstrap cert chain:\t\tlength = %d" LINE_TERM, strlen(certChain));
            else
                CreatorConsole_Printf("Bootstrap cert chain:\tnot set" LINE_TERM);
        }

        CreatorConsole_Puts(LINE_TERM LINE_TERM);
    }
    else
    {
        CreatorConsole_Printf("Error, device server configuration is invalid. Please reset the configuration using the 'factory_reset' command" LINE_TERM);
    }
}

void StandardCommands_SetDeviceName(void)
{
	if (ConfigStore_Config_Read() && ConfigStore_Config_IsValid())
    {
        // Display current device name
        char *deviceName = (char *) ConfigStore_GetDeviceName();
        if (deviceName && strlen(deviceName) > 0)
            CreatorConsole_Printf("Current device name: %s" LINE_TERM, deviceName);
        else
            CreatorConsole_Printf("Current device name: not set" LINE_TERM);

        if (CreatorCommand_PromptUserWithQuery("Are you sure you want to set a new device name: (y/n) "))
        {
            char settingBuffer[CONFIG_STORE_DEFAULT_FIELD_LENGTH + 1];
            if (CreatorCommand_ReadInputStringWithQuery("Enter new device name: ", (uint8_t *) settingBuffer, CONFIG_STORE_DEFAULT_FIELD_LENGTH + 1, false) > 0)
            {
                if (ConfigStore_SetDeviceName(settingBuffer) && ConfigStore_Config_UpdateCheckbyte() && ConfigStore_Config_Write())
                    CreatorConsole_Printf("Setting updated successfully." LINE_TERM);
                else
                    CreatorConsole_Printf("Setting update failed." LINE_TERM);
            }
            else
            {
                CreatorConsole_Printf("Invalid setting value. Aborted" LINE_TERM);
            }
        }
        else
        {
            CreatorConsole_Printf("Command aborted. Setting was not changed." LINE_TERM);
        }
    }
}

void StandardCommands_SetLogConfig(void)
{
    bool settingChanged = false;
    if (ConfigStore_LoggingSettings_Read() && ConfigStore_LoggingSettings_IsValid())
    {
        // Display current Log 'enabled' setting
        bool logEnabled = (uint8_t*) ConfigStore_GetLoggingEnabled();
        CreatorConsole_Printf("Logging currently: ");
        if (logEnabled)
            CreatorConsole_Printf("Enabled" LINE_TERM);
        else
            CreatorConsole_Printf("Disabled" LINE_TERM);

        if (CreatorCommand_PromptUserWithQuery("Modify the above setting? (y/n) "))
        {
            if (ConfigStore_SetLoggingEnabled(CreatorCommand_PromptUserWithQuery("Enable logging? (y/n) ")))
                CreatorConsole_Printf("Setting updated successfully." LINE_TERM);
            else
                CreatorConsole_Printf("Setting update failed." LINE_TERM);

            settingChanged = true;
        }
        // Display current log-level setting
        CreatorActivityLogLevel logLevel = ConfigStore_GetLoggingLevel();
        CreatorConsole_Printf("Logging level: ");
        if (logLevel < CreatorActivityLogLevel_Max)
            CreatorConsole_Printf("%s" LINE_TERM, (char*) ConfigStore_GetLoggingLevelName(logLevel));
        else
            CreatorConsole_Printf("INVALID" LINE_TERM);

        if (CreatorCommand_PromptUserWithQuery("Set a new logging level setting? (y/n) "))
        {
            CreatorConsole_Printf(
            LINE_TERM "Select a new logging level: ");
            uint32_t loggingLevelIndex = 0;
            for (loggingLevelIndex = 0; loggingLevelIndex < CreatorActivityLogLevel_Max; loggingLevelIndex++)
            {
                CreatorConsole_Printf(LINE_TERM "\t%s", ConfigStore_GetLoggingLevelName(loggingLevelIndex));
            }
            CreatorConsole_Printf(LINE_TERM);
            int32_t selection = CreatorCommand_ReadInputIntegerOption(CreatorActivityLogLevel_Max, true, false);
            if (selection > -1)
            {
                if (ConfigStore_SetLoggingLevel(selection))
                {
                    Client_SetLogLevel(selection);
                    CreatorConsole_Printf(LINE_TERM "Setting updated successfully." LINE_TERM);
                    settingChanged = true;
                }
                else
                    CreatorConsole_Printf(LINE_TERM "Setting update failed." LINE_TERM);
            }
            else
            {
                CreatorConsole_Printf(LINE_TERM "Invalid selection. Aborted" LINE_TERM);
            }
        }
        // Display current log-categories settings
        CreatorActivityLogCategory logCategories = ConfigStore_GetLoggingCategories();
        CreatorConsole_Printf("Log categories:\t\t");
        if (logCategories != 0)
        {
            if (logCategories & 1 << CreatorActivityLogCategory_HardwareBoot)
                CreatorConsole_Printf("hwboot ");

            if (logCategories & 1 << CreatorActivityLogCategory_Startup)
                CreatorConsole_Printf("startup ");

            if (logCategories & 1 << CreatorActivityLogCategory_SystemRuntime)
                CreatorConsole_Printf("runtime ");

            if (logCategories & 1 << CreatorActivityLogCategory_App)
                CreatorConsole_Printf("app ");

            if (logCategories & 1 << CreatorActivityLogCategory_Shutdown)
                CreatorConsole_Printf("shutdown ");
        }
        else
        {
            CreatorConsole_Printf("<none selected>");
        }
        CreatorConsole_Printf(LINE_TERM);
        if (CreatorCommand_PromptUserWithQuery("Modify logging categories? (y/n) "))
        {
            uint32_t loggingCategoryIndex = 0;
            CreatorConsole_Printf(LINE_TERM);
            for (loggingCategoryIndex = 0; loggingCategoryIndex < CreatorActivityLogCategory_Max; loggingCategoryIndex++)
            {
                CreatorConsole_Printf("Enable logging category '%s': (y/n) ", ConfigStore_GetLoggingCategoryName(loggingCategoryIndex));
                if (CreatorCommand_PromptUser())
                    logCategories |= 1 << loggingCategoryIndex; // Set the category bit
                else
                    logCategories &= ~(1 << loggingCategoryIndex); // Clear the category bit
            }

            if (ConfigStore_SetLoggingCategories(logCategories))
            {
                CreatorConsole_Printf("Setting updated." LINE_TERM);
                settingChanged = true;
            }
            else
                CreatorConsole_Printf("Setting update failed." LINE_TERM);
        }

        // Save any changes
        if (settingChanged)
        {
            if (ConfigStore_LoggingSettings_UpdateCheckbyte() && ConfigStore_LoggingSettings_Write())
                CreatorConsole_Printf("Logging-settings updated successfully." LINE_TERM);
            else
                CreatorConsole_Printf("Logging-setting update failed." LINE_TERM);
        }
        else
        {
            CreatorConsole_Printf("No logging-settings were changed." LINE_TERM);
        }

    }
}

static void StandardCommands_SetNetworkConfig(void)
{
    if (ConfigStore_Config_Read() && ConfigStore_Config_IsValid())
    {
        // Display current WiFi SSID
        char* wifiSSID = (char *) ConfigStore_GetNetworkSSID();
        CreatorConsole_Printf("Current WiFi network SSID: ");
        if (wifiSSID && strlen(wifiSSID) > 0)
            CreatorConsole_Printf("%s" LINE_TERM, wifiSSID);
        else
            CreatorConsole_Printf("not set" LINE_TERM);

        if (CreatorCommand_PromptUserWithQuery("Do you want to set a new WiFi network SSID: (y/n) "))
        {
            uint8_t settingBuffer[CONFIG_STORE_DEFAULT_FIELD_LENGTH + 1];
            if (CreatorCommand_ReadInputStringWithQuery("Enter new WiFi network SSID: ", settingBuffer, CONFIG_STORE_DEFAULT_FIELD_LENGTH + 1, false) > 0)
            {
                if (ConfigStore_SetNetworkSSID((const char *) settingBuffer) && ConfigStore_Config_UpdateCheckbyte() && ConfigStore_Config_Write())
                    CreatorConsole_Printf(LINE_TERM"Setting updated successfully." LINE_TERM);
                else
                    CreatorConsole_Printf(LINE_TERM "Setting update failed." LINE_TERM);
            }
            else
            {
                CreatorConsole_Printf("Invalid setting value. Aborted" LINE_TERM);
            }
        }
        // Display current encryption type
        WiFiEncryptionType encryption = ConfigStore_GetEncryptionType();
        CreatorConsole_Printf("Current network encryption type: ");
        if (encryption < WiFiEncryptionType_Max)
            CreatorConsole_Printf("%s" LINE_TERM, (char*) ConfigStore_GetEncryptionName(encryption));
        else
            CreatorConsole_Printf("INVALID" LINE_TERM);
        if (CreatorCommand_PromptUserWithQuery("Do you want to set a new WiFi network encryption type: (y/n) "))
        {
            uint8_t encryptionIndex;
            CreatorConsole_Printf("Supported WiFi encryption types: ");
            for (encryptionIndex = 0; encryptionIndex < WiFiEncryptionType_Max; encryptionIndex++)
                CreatorConsole_Printf(LINE_TERM LINE_TAB "%d) %s", encryptionIndex + 1, (char*) ConfigStore_GetEncryptionName(encryptionIndex));

            int32_t selection = CreatorCommand_ReadInputIntegerOptionWithQuery(LINE_TERM "Select a new WiFi encryption type: ", WiFiEncryptionType_Max, false, false);
            if (selection > -1)
            {
                if (ConfigStore_SetNetworkEncryption(selection - 1) && ConfigStore_Config_UpdateCheckbyte() && ConfigStore_Config_Write())
                    CreatorConsole_Printf(LINE_TERM "Setting updated successfully." LINE_TERM);
                else
                    CreatorConsole_Printf(LINE_TERM "Setting update failed." LINE_TERM);
            }
            else
            {
                CreatorConsole_Printf(LINE_TERM "Invalid selection. Aborted" LINE_TERM);
            }
        }
        // Display state of current WiFi Password (if not 'open' encryption)
        if (ConfigStore_GetEncryptionType() != WiFiEncryptionType_Open)
        {
            uint8_t* wifiPassword = (uint8_t*) ConfigStore_GetNetworkPassword();
            CreatorConsole_Printf("Current WiFi network password: ");
            if (wifiPassword && strlen((const char *) wifiPassword) > 0)
            {
                unsigned int index = 0;
                for (index = 0 ; index < strlen(wifiPassword); index++)
                {
                    CreatorConsole_Putc('X');
                }
                CreatorConsole_Printf(LINE_TERM);
            }
            else
                CreatorConsole_Printf("not set" LINE_TERM);
            
            
            
            if (CreatorCommand_PromptUserWithQuery("Do you want to set a new WiFi network password: (y/n) "))
            {
                uint8_t settingBuffer[CONFIG_STORE_DEFAULT_FIELD_LENGTH + 1];
                while (1)
                {
                    if (CreatorCommand_ReadInputStringWithQuery("Enter new WiFi network password: ", settingBuffer, CONFIG_STORE_DEFAULT_FIELD_LENGTH + 1, true) > 0)
                    {
                        uint8_t key_len = strlen(settingBuffer);
                        if ((key_len < MIN_KEY_PHRASE_LENGTH) || (key_len > MAX_KEY_PHRASE_LENGTH))
                        {
                            CreatorConsole_Printf("Entered pass phrase is: %s" LINE_TERM, settingBuffer);
                            CreatorConsole_Printf("WPA Passphrase should contain 8 to 63 characters."LINE_TERM);
                            CreatorConsole_Printf("Please enter WiFi network password again:"LINE_TERM);
                            continue;
                        }

                        if (ConfigStore_SetNetworkPassword((const char*) settingBuffer) && ConfigStore_Config_UpdateCheckbyte() && ConfigStore_Config_Write())
                            CreatorConsole_Printf(LINE_TERM "Setting updated successfully." LINE_TERM);
                        else
                            CreatorConsole_Printf(LINE_TERM "Setting update failed." LINE_TERM);

                        break;
                    }
                    else
                    {
                        CreatorConsole_Printf("Invalid setting value. Aborted" LINE_TERM);
                    }
                } //while - loop
            }
        }
        // Display current addressing scheme
        AddressScheme addressingScheme = ConfigStore_GetAddressingScheme();
        CreatorConsole_Printf("Current addressing scheme: ");
        if (addressingScheme < AddressScheme_Max)
            CreatorConsole_Printf("%s" LINE_TERM, (char*) ConfigStore_GetAddressingSchemeName(addressingScheme));
        else
            CreatorConsole_Printf("INVALID" LINE_TERM);
        if (CreatorCommand_PromptUserWithQuery("Do you want to set a new addressing scheme: (y/n) "))
        {
            uint8_t addressingSchemeIndex;
            CreatorConsole_Printf("Supported WiFi encryption types: ");
            for (addressingSchemeIndex = 0; addressingSchemeIndex < AddressScheme_Max; addressingSchemeIndex++)
                CreatorConsole_Printf(LINE_TERM LINE_TAB "%d) %s", addressingSchemeIndex + 1, (char*) ConfigStore_GetAddressingSchemeName(addressingSchemeIndex));

            int32_t selection = CreatorCommand_ReadInputIntegerOptionWithQuery(LINE_TERM "Select a new addressing scheme: ", AddressScheme_Max, false, false);
            if (selection > -1)
            {
                if (ConfigStore_SetAddressingScheme(selection - 1) && ConfigStore_Config_UpdateCheckbyte() && ConfigStore_Config_Write())
                    CreatorConsole_Printf(LINE_TERM "Setting updated successfully." LINE_TERM);
                else
                    CreatorConsole_Printf(LINE_TERM "Setting update failed." LINE_TERM);
            }
            else
            {
                CreatorConsole_Printf(LINE_TERM "Invalid selection. Aborted" LINE_TERM);
            }
            addressingScheme = ConfigStore_GetAddressingScheme();
        }
        if (addressingScheme == AddressScheme_StaticIP)
        {
            // Display current Static IP
            uint8_t* staticIp = (uint8_t*) ConfigStore_GetStaticIP();
            CreatorConsole_Printf("Current Static IP address: ");
            if (staticIp && strlen((const char *) staticIp) > 0)
                CreatorConsole_Printf("%s" LINE_TERM, staticIp);
            else
                CreatorConsole_Printf("not set" LINE_TERM);

            if (CreatorCommand_PromptUserWithQuery("Do you want to set a new Static IP address: (y/n) "))
            {
                uint8_t settingBuffer[CONFIG_STORE_DEFAULT_FIELD_LENGTH + 1];
                if (CreatorCommand_ReadInputStringWithQuery("Enter new Static IP address: ", settingBuffer, CONFIG_STORE_DEFAULT_FIELD_LENGTH + 1, false) > 0)
                {
                    if (ConfigStore_SetStaticIP((const char*) settingBuffer) && ConfigStore_Config_UpdateCheckbyte() && ConfigStore_Config_Write())
                        CreatorConsole_Printf(LINE_TERM"Setting updated successfully." LINE_TERM);
                    else
                        CreatorConsole_Printf(LINE_TERM "Setting update failed." LINE_TERM);
                }
                else
                {
                    CreatorConsole_Printf("Invalid setting value. Aborted" LINE_TERM);
                }
            }
            // Display current Static Netmask
            uint8_t* staticNetmask = (uint8_t*) ConfigStore_GetStaticNetmask();
            CreatorConsole_Printf("Current Static netmask: ");
            if (staticNetmask && strlen((const char *) staticNetmask) > 0)
                CreatorConsole_Printf("%s" LINE_TERM, staticNetmask);
            else
                CreatorConsole_Printf("not set" LINE_TERM);

            if (CreatorCommand_PromptUserWithQuery("Do you want to set a new Static netmask: (y/n) "))
            {
                uint8_t settingBuffer[CONFIG_STORE_DEFAULT_FIELD_LENGTH + 1];
                if (CreatorCommand_ReadInputStringWithQuery("Enter new Static netmask: ", settingBuffer, CONFIG_STORE_DEFAULT_FIELD_LENGTH + 1, false) > 0)
                {
                    if (ConfigStore_SetStaticNetmask((const char*) settingBuffer) && ConfigStore_Config_UpdateCheckbyte() && ConfigStore_Config_Write())
                        CreatorConsole_Printf(
                        LINE_TERM"Setting updated successfully." LINE_TERM);
                    else
                        CreatorConsole_Printf(
                        LINE_TERM "Setting update failed." LINE_TERM);
                }
                else
                {
                    CreatorConsole_Printf("Invalid setting value. Aborted" LINE_TERM);
                }
            }
            // Display current Static Gateway
            uint8_t* staticGateway = (uint8_t*) ConfigStore_GetStaticGateway();
            CreatorConsole_Printf("Current Static gateway: ");
            if (staticGateway && strlen((const char *) staticGateway) > 0)
                CreatorConsole_Printf("%s" LINE_TERM, staticGateway);
            else
                CreatorConsole_Printf("not set" LINE_TERM);
            if (CreatorCommand_PromptUserWithQuery("Do you want to set a new Static gateway: (y/n) "))
            {
                uint8_t settingBuffer[CONFIG_STORE_DEFAULT_FIELD_LENGTH + 1];
                if (CreatorCommand_ReadInputStringWithQuery("Enter new Static gateway: ", settingBuffer, CONFIG_STORE_DEFAULT_FIELD_LENGTH + 1, false) > 0)
                {
                    if (ConfigStore_SetStaticGateway((const char*) settingBuffer) && ConfigStore_Config_UpdateCheckbyte() && ConfigStore_Config_Write())
                        CreatorConsole_Printf(LINE_TERM "Setting updated successfully." LINE_TERM);
                    else
                        CreatorConsole_Printf(LINE_TERM "Setting update failed." LINE_TERM);
                }
                else
                {
                    CreatorConsole_Printf("Invalid setting value. Aborted" LINE_TERM);
                }
            }

            // Display current Static DNS
            uint8_t* staticDns = (uint8_t*) ConfigStore_GetStaticDNS();
            CreatorConsole_Printf("Current Static DNS: ");
            if (staticDns && strlen((const char *) staticDns) > 0)
                CreatorConsole_Printf("%s" LINE_TERM, staticDns);
            else
                CreatorConsole_Printf("not set" LINE_TERM);

            if (CreatorCommand_PromptUserWithQuery("Do you want to set a new Static DNS: (y/n) "))
            {
                uint8_t settingBuffer[CONFIG_STORE_DEFAULT_FIELD_LENGTH + 1];
                if (CreatorCommand_ReadInputStringWithQuery("Enter new Static DNS: ", settingBuffer, CONFIG_STORE_DEFAULT_FIELD_LENGTH + 1, false) > 0)
                {
                    if (ConfigStore_SetStaticDNS((const char*) settingBuffer) && ConfigStore_Config_UpdateCheckbyte() && ConfigStore_Config_Write())
                        CreatorConsole_Printf(LINE_TERM "Setting updated successfully." LINE_TERM);
                    else
                        CreatorConsole_Printf(LINE_TERM "Setting update failed." LINE_TERM);
                }
                else
                {
                    CreatorConsole_Printf("Invalid setting value. Aborted" LINE_TERM);
                }
            }
        }
        
        // Prompt user as to whether they would like to boot into configuration mode
        // TODO - move to separate command
        if (CreatorCommand_PromptUserWithQuery("Do you want to force this device into application mode next time it reboots: (y/n) "))
        {
            if (AppConfig_CheckValidAppConfig(false))
            {
                if (ConfigStore_SetResetToConfigurationMode(false) && 
                    ConfigStore_SetNetworkConfigSet(true) &&
                    ConfigStore_Config_UpdateCheckbyte() && 
                    ConfigStore_Config_Write())
                {
                    // Next boot should be into application mode
                }
            }
            else
            {
                char* wifiSsid = (char *) ConfigStore_GetNetworkSSID();
                char* wifiPassword = (char *) ConfigStore_GetNetworkPassword();
                char* bootUrl = (char*) ConfigStore_GetBootstrapURL();
                
                if (0 == (strlen(wifiSsid)))
                {
                    CreatorConsole_Printf("Note: Network SSID was not set. Please set Network SSID.\n\r\n\r");
                }
                if (0 == (strlen(wifiPassword)))
                {
                    CreatorConsole_Printf("Note: Network Password was not set. Please set Network Password.\n\r\n\r");
                }
                if (0 == (strlen(bootUrl)))
                {
                    CreatorConsole_Printf("Note: The Bootstrap URL has not been set yet. You will need to set this before you can switch this device into application mode when it reboots.\n\r\n\r");
                }
            }
        }
        else
        {
            SYS_ERROR(SYS_ERROR_WARN, "system_command_startapp: Warning, Could not clear 'boot to configuration mode' flag.");
        }
    }
}

static void StandardCommands_SetDeviceServerConfig(void)
{
    if (ConfigStore_DeviceServerConfig_Read() && ConfigStore_DeviceServerConfig_IsValid())
    {
        // Display current bootstrap URL
        char* bootstrapURL = (char *) ConfigStore_GetBootstrapURL();
        CreatorConsole_Printf("Current bootstrap URL: ");
        if (bootstrapURL && strlen(bootstrapURL) > 0)
            CreatorConsole_Printf("%s" LINE_TERM, bootstrapURL);
        else
            CreatorConsole_Printf("not set" LINE_TERM);

        if (CreatorCommand_PromptUserWithQuery("Do you want to set a new bootstrap URL: (y/n) "))
        {
            uint8_t settingBuffer[CONFIG_STORE_DEFAULT_FIELD_LENGTH + 1];
            if (CreatorCommand_ReadInputStringWithQuery("Enter new bootstrap URL: ", settingBuffer, CONFIG_STORE_DEFAULT_FIELD_LENGTH + 1, false) > 0)
            {
                if (ConfigStore_SetBootstrapURL((const char *) settingBuffer) && ConfigStore_DeviceServerConfig_UpdateCheckbyte() && ConfigStore_DeviceServerConfig_Write())
                    CreatorConsole_Printf(LINE_TERM"Setting updated successfully." LINE_TERM);
                else
                    CreatorConsole_Printf(LINE_TERM "Setting update failed." LINE_TERM);
            }
            else
            {
                CreatorConsole_Printf("Invalid setting value. Aborted" LINE_TERM);
            }
        }
        // Display current security mode
        ServerSecurityMode securityMode = ConfigStore_GetSecurityMode();
        CreatorConsole_Printf("Current security mode: ");
        if (securityMode < ServerSecurityMode_Max)
            CreatorConsole_Printf("%s" LINE_TERM, (char*) ConfigStore_GetSecurityModeName(securityMode));
        else
            CreatorConsole_Printf("INVALID" LINE_TERM);
        if (CreatorCommand_PromptUserWithQuery("Do you want to set a new security mode: (y/n) "))
        {
            uint8_t index;
            CreatorConsole_Printf("Supported security modes: ");
            for (index = 0; index < ServerSecurityMode_Max; index++)
                CreatorConsole_Printf(LINE_TERM LINE_TAB "%d) %s", index + 1, (char*) ConfigStore_GetSecurityModeName(index));

            int32_t selection = CreatorCommand_ReadInputIntegerOptionWithQuery(LINE_TERM "Select a new security mode: ", ServerSecurityMode_Max, false, false);
            if (selection > -1)
            {
                if (ConfigStore_SetSecurityMode(selection - 1) && ConfigStore_DeviceServerConfig_UpdateCheckbyte() && ConfigStore_DeviceServerConfig_Write())
                    CreatorConsole_Printf(LINE_TERM "Setting updated successfully." LINE_TERM);
                else
                    CreatorConsole_Printf(LINE_TERM "Setting update failed." LINE_TERM);
            }
            else
            {
                CreatorConsole_Printf(LINE_TERM "Invalid selection. Aborted" LINE_TERM);
            }
        }

        // Prompt user as to whether they would like to boot into configuration mode
        // TODO - move to separate command
        if (CreatorCommand_PromptUserWithQuery("Do you want to force this device into application mode next time it reboots: (y/n) "))
        {
            if (AppConfig_CheckValidAppConfig(false))
            {
                if (ConfigStore_SetResetToConfigurationMode(false) && 
                    ConfigStore_SetNetworkConfigSet(true) &&
                    ConfigStore_Config_UpdateCheckbyte() && 
                    ConfigStore_Config_Write())
                {
                    // Next boot should be into application mode
                }
            }
            else
            {
                uint8_t* bootUrl = (uint8_t*) ConfigStore_GetBootstrapURL();
                char* wifiSsid = (char *) ConfigStore_GetNetworkSSID();
                char* wifiPassword = (char *) ConfigStore_GetNetworkPassword();
                if (0 == (strlen(bootUrl)))
                {
                    CreatorConsole_Printf("Note: The Bootstrap URL has not been set yet. You will need to set this before you can switch this device into application mode when it reboots.\n\r\n\r");
                }
                if (0 == (strlen(wifiSsid)))
                {
                    CreatorConsole_Printf("Note: Network SSID was not set. Please set Network SSID.\n\r\n\r");
                }
                if (0 == (strlen(wifiPassword)))
                {
                    CreatorConsole_Printf("Note: Network Password was not set. Please set Network Password.\n\r\n\r");
                }
            }
        }
        else
        {
            SYS_ERROR(SYS_ERROR_WARN, "system_command_startapp: Warning, Could not clear 'boot to configuration mode' flag.");
        }
    }
}

static void StandardCommands_ShowWiFireDetails(void)
{
    if (ConfigStore_Config_Read() && ConfigStore_Config_IsValid())
    {
        // Device Name
        char *deviceName = (char *) ConfigStore_GetDeviceName();
        if (deviceName && strlen(deviceName))
            CreatorConsole_Printf("Device Name:\t\t%s" LINE_TERM, deviceName);
        else
            CreatorConsole_Printf("Device Name:\t\tunknown" LINE_TERM);

        // Device Type
        char *deviceType = (char *) ConfigStore_GetDeviceType();
        if (deviceType && strlen(deviceType))
            CreatorConsole_Printf("Device Type:\t\t%s" LINE_TERM, deviceType);
        else
            CreatorConsole_Printf("Device Type:\t\tunknown" LINE_TERM);

        // Device MAC address
        char *mac = (char *) ConfigStore_GetMacAddress();
        if (mac && strlen(mac))
            CreatorConsole_Printf("MAC:\t\t\t%s" LINE_TERM, mac);
        else
            CreatorConsole_Printf("MAC:\t\t\tunknown" LINE_TERM);

        // Device Serial number
        #define serialNumberBufferLen	(17)
        char deviceSerialNumber[serialNumberBufferLen];
        DeviceSerial_GetCpuSerialNumberHexString (deviceSerialNumber, serialNumberBufferLen);
        CreatorConsole_Printf("MCU Serial number:\t%s" LINE_TERM, deviceSerialNumber);

        // Microchip DeviceID
        uint32_t devID = DEVID & 0x0FFFFFFF;
        CreatorConsole_Printf("MCU Device ID:\t\t0x%08X" LINE_TERM, devID);

        // Microchip DeviceID
        uint32_t revision = (DEVID&0xF0000000) >> 28;
        CreatorConsole_Printf("MCU Device Revision:\tRev %d" LINE_TERM, revision);
        // SoftAP SSID setting
        char *softApSSID = (char *) ConfigStore_GetSoftAPSSID();
        if (softApSSID && strlen(softApSSID))
            CreatorConsole_Printf("SoftAP SSID:\t\t%s" LINE_TERM, softApSSID);
        else
            CreatorConsole_Printf("SoftAP SSID:\t\tnot set" LINE_TERM);

        // SoftAP password setting
        char *softApPassword = (char *) ConfigStore_GetSoftAPPassword();
        if (softApPassword && strlen(softApPassword))
            CreatorConsole_Printf("SoftAP Password:\t%s" LINE_TERM, softApPassword);
        else
            CreatorConsole_Printf("SoftAP Password:\tnot set" LINE_TERM);

        CreatorConsole_Printf(LINE_TERM LINE_TERM);
	}
}
