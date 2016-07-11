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

#include <stdbool.h>
#include <stddef.h>
#include "bsp_config.h"
#include "driver/nvm/drv_nvm.h"
#include "tcpip/tcpip.h"
#include "driver/wifi/mrf24w/drv_wifi.h"
#include "driver/wifi/mrf24w/src/drv_wifi_config_data.h"
#include "system/reset/sys_reset.h"


#include "app_config.h"
#include "config_store.h"
#include "ui_control.h"
#include "standard_commands.h"
#include "creator/creatorcore.h"


#ifndef LIB_VERSION
#define LIB_VERSION developer
#endif

#ifndef LIB_VERSIONDATE
#define LIB_VERSIONDATE date
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define	CONFIG_MODE_BTN_PRESS_DEBOUNCE_SECONDS (5)

#define WEP104_KEY_LENGTH 26
#define DATETIME_MINIMUM_VALUE 1401962400   // 2014-06-05 10:00:00 GMT
#define BASE_ADDRESS 0xBD1F0000

#define WF_DEFAULT_SSID_NAME_PREFIX "WiFire_"
#define	MAC_ADDRESS_LENGTH (12)

const uint8_t __attribute__((address(BASE_ADDRESS))) NVS_MEM_FLASH_RESERVE[DRV_NVM_PAGE_SIZE] = {0};

static uint32_t _UptimeSecs = 0;

static bool _DeviceIsOnline = false;
static bool _RunningInConfigurationMode = false;

AppInfo *_AppInfo = NULL;
DRV_WIFI_SOFT_AP_STATES lastState = -1;

// UI Control
bool AppConfig_InitAppCoreTasks(void);
void AppConfig_InitCreatorCommandHandler(void);
static void AppConfig_UI_Task(void);

// Network
static uint8_t SetWEPKey(const char *password);

// Helpers
static uint8_t GetHexValue(char text);

bool AppConfig_CheckForConfigurationModeRebootButtonPress(void)
{
    bool result = false;
    // Debounce switch presses for toggling the runtime mode
    static bool started = false;
    static bool stopped = true;
    volatile bool sw1State = (BSP_SwitchStateGet(BSP_SWITCH_1) == BSP_SWITCH_STATE_PRESSED);
    volatile bool sw2State = (BSP_SwitchStateGet(BSP_SWITCH_2) == BSP_SWITCH_STATE_PRESSED);
    time_t currentTime;

    if (sw1State && sw2State)
    {
        Creator_GetTime(&currentTime);
        static time_t pressTime = 0;
        if (!started)
        {
            pressTime = currentTime;
            started = true;
        }

        if (stopped && (int32_t) currentTime - (int32_t) pressTime >= CONFIG_MODE_BTN_PRESS_DEBOUNCE_SECONDS)
        {
            result = true;
            stopped = false;
        }
    }
    else
    {
        started = false;
        stopped = true;
    }
    return result;
}

AppInfo *AppConfig_GetAppInfo(void)
{
    return _AppInfo;
}

char *AppConfig_GetVersion(void)
{
    return TOSTRING(LIB_VERSION);
}

char *AppConfig_GetVersionDate(void)
{
    return TOSTRING(LIB_VERSIONDATE);
}

void WolfDebug(const int logLevel, const char * const logMessage)
{
    CreatorConsole_Puts(logMessage);
    CreatorConsole_Puts("\r\n");
}

void AppConfig_Initialise(AppInfo *info)
{
    _AppInfo = info;

    AppConfig_InitCreatorCommandHandler();
    UIControl_SetUIState(AppUIState_None);

    //wolfSSL_SetLoggingCb(WolfDebug);
    //wolfSSL_Debugging_ON();
    AppConfig_InitAppCoreTasks();
}

bool AppConfig_IsDeviceOnline(void)
{
    return _DeviceIsOnline;
}

bool AppConfig_IsRunningInConfigurationMode(void)
{
    return _RunningInConfigurationMode;
}

void AppConfig_SetDeviceOnline(bool deviceStatus)
{
    _DeviceIsOnline = deviceStatus;
}

void AppConfig_NetworkInitialise(void)
{
    bool configDefault = false;
    bool logSettingsDefault = false;
    bool deviceServerConfig = false;

    //Unfortunately defaults to connected
    DRV_WIFI_MGMT_INDICATE_SOFT_AP_EVENT *event = DRV_WIFI_SoftApEventInfoGet();
    if (event)
    {
        event->event = -1;
    }

    ConfigStore_Initialize();
    // TODO - remove debug
//   	CreatorConsole_Printf("DeviceConfig size = %d, LogConfig size = %d\r\n", sizeof(ConfigStruct), sizeof(LoggingSettingsStruct));
//   	CreatorConsole_Printf("LOGGINGSETTINGS_PAGEOFFSET = %d\r\n", LOGGINGSETTINGS_PAGEOFFSET);
//   	CreatorConsole_Printf("DRV_NVM_PAGE_SIZE  = %d\r\n", DRV_NVM_PAGE_SIZE);

    if (!ConfigStore_Config_Read())
        CreatorConsole_Puts("ERROR: Could not read config_store memory.\r\n");

    if (!ConfigStore_Config_IsValid())
    {
        if (!ConfigStore_Config_IsMagicValid())
            CreatorConsole_Puts("\r\nWriting default device configuration for first-time use...");
        else
            CreatorConsole_Puts("\r\nDevice configuration invalid. Re-writing default configuration...");

        configDefault = true;
        if (!ConfigStore_Config_ResetToDefaults())
            CreatorConsole_Puts("ERROR: Could not reset config to defaults\r\n");

        if (!ConfigStore_Config_Write())
            CreatorConsole_Puts("ERROR: Could not write default config\r\n");

        CreatorConsole_Puts(" Done\r\n");
    }

    if (!ConfigStore_LoggingSettings_Read())
        CreatorConsole_Puts("ERROR: Could not read loggingSettings.\r\n");

    if (!ConfigStore_LoggingSettings_IsValid())
    {
        if (!ConfigStore_LoggingSettings_IsMagicValid())
            CreatorConsole_Puts("\r\nWriting default device logging-settings for first-time use...");
        else
            CreatorConsole_Puts("\r\nDevice logging-settings invalid. Re-writing defaults...");

        logSettingsDefault = true;
        if (!ConfigStore_LoggingSettings_ResetToDefaults())
            CreatorConsole_Puts("ERROR: Could not reset config to defaults\r\n");

        if (!ConfigStore_LoggingSettings_Write())
            CreatorConsole_Puts("ERROR: Could not write default config\r\n");

        CreatorConsole_Puts(" Done\r\n");
    }

    CreatorLogLevel level = ConfigStore_GetLoggingLevel();
    CreatorLog_SetLevel(level);
    Client_SetLogLevel(level);

    if (!ConfigStore_DeviceServerConfig_Read())
        CreatorConsole_Puts("ERROR: Could not read device server settings.\r\n");

    if (!ConfigStore_DeviceServerConfig_IsValid())
    {
        if (!ConfigStore_DeviceServerConfig_IsMagicValid())
            CreatorConsole_Puts("\r\nWriting default device server settings for first-time use...");
        else
            CreatorConsole_Puts("\r\nDevice server settings invalid. Re-writing defaults...");

        deviceServerConfig = true;
        if (!ConfigStore_DeviceServerConfig_ResetToDefaults())
            CreatorConsole_Puts("ERROR: Could not reset device server config to defaults\r\n");

        if (!ConfigStore_DeviceServerConfig_Write())
            CreatorConsole_Puts("ERROR: Could not write default device server config\r\n");

        CreatorConsole_Puts(" Done\r\n");
    }

    CreatorTimer_SetTicksPerSecond(1000);

    // Check date/time has valid minimum value
    // TODO - could use NTP to get time (in app mode), or mobile app could set time (in config mode)
    time_t time = Creator_GetTime(NULL);
    if (time < DATETIME_MINIMUM_VALUE)
    {
        Creator_SetTime(DATETIME_MINIMUM_VALUE);
    }
    else
    {
        Creator_SetTime(time);	// Kick start the date/time support
    }
    
    // Add initial activity log entries (Note: must be after date/time and logging settings initialised)
    if (configDefault && logSettingsDefault && deviceServerConfig)
    {
        Creator_Log(CreatorLogLevel_Warning, "Default configuration settings set for first-time use");
    }
    else
    {
        if (configDefault)
            Creator_Log(CreatorLogLevel_Error, "Creator configuration lost, default values set");
        if (logSettingsDefault)
            Creator_Log(CreatorLogLevel_Error, "Logging settings lost, default values set");
        if (deviceServerConfig)
            Creator_Log(CreatorLogLevel_Error, "Device server configuration lost, default values set");
    }

    if ((BSP_SwitchStateGet(BSP_SWITCH_1) == BSP_SWITCH_STATE_PRESSED) && (BSP_SwitchStateGet(BSP_SWITCH_2) == BSP_SWITCH_STATE_PRESSED))
        _RunningInConfigurationMode = true;

    // Only run in application mode if device has valid configuration
    if (!_RunningInConfigurationMode && (!ConfigStore_Config_IsValid() || ConfigStore_StartInConfigMode() || !AppConfig_CheckValidAppConfig(false)))
        _RunningInConfigurationMode = true;

    int numberOfNetworkInterfaces = TCPIP_STACK_NumberOfNetworksGet();
    int index;
    TCPIP_NET_HANDLE networkHandle = 0;
    do
    {
        CreatorThread_SleepMilliseconds(NULL, 500);
        for (index = 0; index < numberOfNetworkInterfaces; index++)
        {
            networkHandle = TCPIP_STACK_IndexToNet(index);
            if (TCPIP_STACK_NetIsUp(networkHandle))
            {
                break;
            }
        }
    } while (index == numberOfNetworkInterfaces);

    CreatorConsole_Puts("Done\r\n");

    TCPIP_DHCP_Disable(networkHandle);
    TCPIP_DNS_Disable(networkHandle, true);
    TCPIP_DHCPS_Disable(networkHandle);
    TCPIP_DNSS_Disable(networkHandle);

    if (!ConfigStore_SoftAPSSIDValid())
    {
        const unsigned int MAC_ADDRESS_UNIQUE_PORTION_LENGTH = 6;
        char *softAPSSID = (char *) ConfigStore_GetSoftAPSSID();
        unsigned int baseStrLen = strlen(WF_DEFAULT_SSID_NAME_PREFIX) * sizeof(char);
        memset(softAPSSID, 0, CONFIG_STORE_DEFAULT_FIELD_LENGTH);
        memcpy(softAPSSID, WF_DEFAULT_SSID_NAME_PREFIX, baseStrLen);
        char macStr[MAC_ADDRESS_LENGTH];
        memset(macStr, 0, sizeof(macStr));
        uint8_t mac[MAC_ADDRESS_LENGTH / 2];
        DRV_WIFI_MacAddressGet(mac);
        unsigned int i = 0;
        for (i = 0; i < MAC_ADDRESS_LENGTH / 2; i++)
            sprintf(macStr + (i * 2 * sizeof(char)), "%02X", mac[i]);
        // Store device MAC Address so it doesn't need to be retrieved more than once
        ConfigStore_SetMacAddress(macStr);
        // Use unique portion of mac address to build SSID (last three bytes)
        strncpy((char*) softAPSSID + baseStrLen, macStr + MAC_ADDRESS_LENGTH - MAC_ADDRESS_UNIQUE_PORTION_LENGTH, MAC_ADDRESS_UNIQUE_PORTION_LENGTH);

        // Set the device's name to the same as the SoftAP SSID if it is still the blank value
        if (strcmp(ConfigStore_GetDeviceName(), CREATOR_BLANK_DEVICE_NAME) == 0)
            ConfigStore_SetDeviceName(softAPSSID);

        // Update the Device's configuration information
        ConfigStore_Config_UpdateCheckbyte();
        if (!ConfigStore_Config_Write())
            CreatorConsole_Puts("ERROR: Could not write default config");
    }

    uint8_t networkType;
    uint8_t securityType = 0;
    uint8_t connectionState;
    DRV_WIFI_NetworkTypeGet(&networkType);

    if (networkType == DRV_WIFI_NETWORK_TYPE_ADHOC)
    {
        do
        {
            CreatorThread_SleepMilliseconds(NULL, 100);
            DRV_WIFI_ConnectionStateGet(&connectionState);
        } while ((connectionState == DRV_WIFI_CSTATE_CONNECTION_IN_PROGRESS) || (connectionState == DRV_WIFI_CSTATE_RECONNECTION_IN_PROGRESS));
    }
    else if (networkType == DRV_WIFI_NETWORK_TYPE_SOFT_AP)
    {
        do
        {
            CreatorThread_SleepMilliseconds(NULL, 100);
            DRV_WIFI_ConnectionStateGet(&connectionState);
        } while ((connectionState != DRV_WIFI_CSTATE_CONNECTION_IN_PROGRESS) && (connectionState == DRV_WIFI_CSTATE_CONNECTION_PERMANENTLY_LOST));
    }

    DRV_WIFI_ReconnectModeSet(0, DRV_WIFI_DO_NOT_ATTEMPT_TO_RECONNECT, 40, DRV_WIFI_DO_NOT_ATTEMPT_TO_RECONNECT);

    DRV_WIFI_Disconnect();

    do
    {
        CreatorThread_SleepMilliseconds(NULL, 100);
        DRV_WIFI_ConnectionStateGet(&connectionState);
    } while (connectionState != DRV_WIFI_CSTATE_NOT_CONNECTED);

    if (_AppInfo)
    {
        CreatorConsole_Puts("\r\n --- ");
        CreatorConsole_Puts(_AppInfo->ApplicationName);
        CreatorConsole_Puts(" v");
        CreatorConsole_Puts(_AppInfo->ApplicationVersion);
        CreatorConsole_Puts(" ---\r\n");
    }
    const char *networkSSID;
    const char *softAPPPassword = NULL;
    if (_RunningInConfigurationMode)
    {
        networkType = DRV_WIFI_NETWORK_TYPE_SOFT_AP;
        CreatorConsole_Puts("              [Configuration Mode]\r\n\r\n\r\n");
        networkSSID = ConfigStore_GetSoftAPSSID();
        const char *mac = ConfigStore_GetSoftAPSSID();
        int networkSSIDLength = strlen(networkSSID);
        DRV_WIFI_SsidSet((uint8_t *) networkSSID, networkSSIDLength);
        softAPPPassword = ConfigStore_GetSoftAPPassword();
        securityType = SetWEPKey(softAPPPassword);
        DRV_WIFI_NetworkTypeSet(DRV_WIFI_NETWORK_TYPE_SOFT_AP);

        while (DRV_WIFI_ContextLoad() == TCPIP_MAC_RES_PENDING)
        {
            CreatorThread_SleepMilliseconds(NULL, 50);
        }
    }
    else
    {
        networkType = DRV_WIFI_NETWORK_TYPE_INFRASTRUCTURE;
        UIControl_SetUIState(AppUIState_AppInitConnectingToNetwork);
        CreatorConsole_Puts("               [Application Mode]\r\n\r\n\r\n");

        uint8_t channelList[] = { };
        DRV_WIFI_ChannelListSet(channelList, sizeof(channelList));

        DRV_WIFI_ReconnectModeSet(DRV_WIFI_RETRY_FOREVER,         // retry forever to connect to Wi-Fi network
                DRV_WIFI_ATTEMPT_TO_RECONNECT,  // reconnect on deauth from AP
                40,                             // beacon timeout is 40 beacon periods
                DRV_WIFI_ATTEMPT_TO_RECONNECT); // reconnect on beacon timeout

        networkSSID = ConfigStore_GetNetworkSSID();
        int networkSSIDLength = strlen(networkSSID);
        DRV_WIFI_SsidSet((uint8_t *) networkSSID, networkSSIDLength);

        WiFiEncryptionType encryptionType = ConfigStore_GetEncryptionType();
        switch (encryptionType) {
            case WiFiEncryptionType_WEP:
            {
                const char *wepKey = ConfigStore_GetNetworkPassword();
                securityType = SetWEPKey(wepKey);
            }
                break;
            case WiFiEncryptionType_Open:
                DRV_WIFI_SecurityOpenSet();
                securityType = DRV_WIFI_SECURITY_OPEN;
                break;
            case WiFiEncryptionType_WPA:
            case WiFiEncryptionType_WPA2:
            default:
            {
                DRV_WIFI_WPA_CONTEXT context;
                if (encryptionType == WiFiEncryptionType_WPA)
                    context.wpaSecurityType = DRV_WIFI_SECURITY_WPA_WITH_KEY; // DRV_WIFI_SECURITY_WPA_WITH_PASS_PHRASE;
                else
                    context.wpaSecurityType = DRV_WIFI_SECURITY_WPA2_WITH_KEY; //DRV_WIFI_SECURITY_WPA2_WITH_PASS_PHRASE;
                const char *passPhrase = ConfigStore_GetNetworkPassword();
                int keyLength = strlen(passPhrase);
                memcpy(context.keyInfo.key, passPhrase, keyLength);
                context.keyInfo.keyLength = keyLength;

                while (TCPIP_MAC_RES_OK != DRV_WIFI_KeyDerive(keyLength, context.keyInfo.key, networkSSIDLength, networkSSID))
                    ;
                context.keyInfo.keyLength = 32;

                DRV_WIFI_SecurityWpaSet(&context);
                securityType = context.wpaSecurityType;
            }
                break;
        }
        DRV_WIFI_NetworkTypeSet(DRV_WIFI_NETWORK_TYPE_INFRASTRUCTURE);
    }

    CreatorConsole_Puts("==========================\r\n");
    CreatorConsole_Puts("*** WiFi Configuration ***\r\n");
    CreatorConsole_Puts("==========================\r\n");
    CreatorConsole_Puts("MAC:\t\t");
    CreatorConsole_Puts(ConfigStore_GetMacAddress());
    CreatorConsole_Puts("\r\nSSID:\t\t");
    CreatorConsole_Puts(networkSSID);
    if (softAPPPassword)
    {
        CreatorConsole_Puts("\r\nPassword:\t");
        CreatorConsole_Puts(softAPPPassword);

    }
    CreatorConsole_Puts("\r\nNetwork Type:\t");

    switch (networkType) {
        case DRV_WIFI_NETWORK_TYPE_INFRASTRUCTURE:
            CreatorConsole_Puts("Infrastructure");
            break;
        case DRV_WIFI_NETWORK_TYPE_ADHOC:
            CreatorConsole_Puts("AdHoc");
            break;
        case DRV_WIFI_NETWORK_TYPE_SOFT_AP:
            CreatorConsole_Puts("SoftAP");
            break;
    }
    CreatorConsole_Puts("\r\nSecurity:\t");

    switch (securityType) {
        case DRV_WIFI_SECURITY_OPEN:
            CreatorConsole_Puts("Open");
            break;
        case DRV_WIFI_SECURITY_WEP_40:
            CreatorConsole_Puts("WEP40");
            break;
        case DRV_WIFI_SECURITY_WEP_104:
            CreatorConsole_Puts("WEP104");
            break;
        case DRV_WIFI_SECURITY_WPA_WITH_KEY:
        case DRV_WIFI_SECURITY_WPA_WITH_PASS_PHRASE:
            CreatorConsole_Puts("WPA");
            break;
        case DRV_WIFI_SECURITY_WPA2_WITH_KEY:
        case DRV_WIFI_SECURITY_WPA2_WITH_PASS_PHRASE:
            CreatorConsole_Puts("WPA2");
            break;
        case DRV_WIFI_SECURITY_WPA_AUTO_WITH_KEY:
        case DRV_WIFI_SECURITY_WPA_AUTO_WITH_PASS_PHRASE:
            CreatorConsole_Puts("WPA AUTO");
            break;
    }

    DRV_WIFI_PsPollDisable();
    DRV_WIFI_Connect();

    CreatorConsole_Puts("\r\n\r\n");

    if (_RunningInConfigurationMode)
    {
        do
        {
            CreatorThread_SleepMilliseconds(NULL, 100);
            DRV_WIFI_ConnectionStateGet(&connectionState);
        } while ((connectionState != DRV_WIFI_CSTATE_CONNECTION_IN_PROGRESS) && (connectionState != DRV_WIFI_CSTATE_RECONNECTION_IN_PROGRESS));
        TCPIP_DHCPS_Enable(networkHandle);
        TCPIP_DNSS_Enable(networkHandle);
        UIControl_SetUIState(AppUIState_SoftApNotConnected);
        IPV4_ADDR ipAdd;
        ipAdd.Val = TCPIP_STACK_NetAddress(networkHandle);
        CreatorConsole_Puts(TCPIP_STACK_NetNameGet(networkHandle));
        CreatorConsole_Puts(" IP Address: ");
        CreatorConsole_Printf("%d.%d.%d.%d \r\n", ipAdd.v[0], ipAdd.v[1], ipAdd.v[2], ipAdd.v[3]);
    }
    else
    {
        CreatorConsole_Printf("\n\rConnecting to WiFi network...\n\r");
        bool displayedConnectionWarningToUser = false;
        do
        {
            CreatorThread_SleepMilliseconds(NULL, 100);
            DRV_WIFI_ConnectionStateGet(&connectionState);

            if (!displayedConnectionWarningToUser)
            {
                SYS_UPTIME uptime;
                AppConfig_Uptime(&uptime);
                if (uptime.Seconds >= 30 && !AppConfig_IsDeviceOnline())
                {
                    CreatorConsole_Printf("\n\r\n\r");
                    CreatorConsole_Printf("\n\r**********************************************************************");
                    CreatorConsole_Printf("\n\r* Your WiFire is taking a long time to connect to your WiFi network. *");
                    CreatorConsole_Printf("\n\r* Please check your network settings are correctly configured.       *");
                    CreatorConsole_Printf("\n\r*                                                                    *");
                    CreatorConsole_Printf("\n\r* Hold BTN1 and BTN2 whilst pressing the RESET button on your WiFire *");
                    CreatorConsole_Printf("\n\r* to restart it in Configuration mode.                               *");
                    CreatorConsole_Printf("\n\r*                                                                    *");
                    CreatorConsole_Printf("\n\r* You can then review and change your settings if required.          *");
                    CreatorConsole_Printf("\n\r**********************************************************************");
                    CreatorConsole_Printf("\n\r\n\r");
                    displayedConnectionWarningToUser = true;
                }
            }

        } while ((connectionState != DRV_WIFI_CSTATE_CONNECTED_INFRASTRUCTURE));
        TCPIP_DHCP_Enable(networkHandle);
        IPV4_ADDR ipAdd;
        ipAdd.Val = TCPIP_STACK_NetAddress(networkHandle);
        while (ipAdd.Val == 0x1901A8C0)
        {
            CreatorThread_SleepMilliseconds(NULL, 500);
            ipAdd.Val = TCPIP_STACK_NetAddress(networkHandle);
        }
        CreatorConsole_Puts(TCPIP_STACK_NetNameGet(networkHandle));
        CreatorConsole_Puts(" IP Address: ");
        CreatorConsole_Printf("%d.%d.%d.%d \r\n", ipAdd.v[0], ipAdd.v[1], ipAdd.v[2], ipAdd.v[3]);
        TCPIP_DNS_Enable(networkHandle, TCPIP_DNS_ENABLE_PREFERRED);
        UIControl_SetUIState(AppUIState_AppInitConnectedToNetwork);
    }
    CreatorConsole_Puts("\r\nConnected\r\n");

    //	IPV4_ADDR ipAdd;
    //	ipAdd.Val = TCPIP_STACK_NetAddress(networkHandle);
    //	CreatorConsole_Puts(TCPIP_STACK_NetNameGet(networkHandle));
    //	CreatorConsole_Puts(" IP Address: ");
    //	CreatorConsole_Printf("%d.%d.%d.%d \r\n", ipAdd.v[0], ipAdd.v[1], ipAdd.v[2], ipAdd.v[3]);

}

void AppConfig_CreatorInitialise(void)
{
    if (!_RunningInConfigurationMode)
    {
        CreatorCore_Initialise();
    }
}

static uint8_t GetHexValue(char text)
{
    uint8_t result = 0;
    if ((text <= '9') && (text >= '0'))
    {
        result = ((char) text) - ((char) '0');
    }
    else if ((text <= 'f') && (text >= 'a'))
    {
        result = (((char) text) - ((char) 'a')) + 10;
    }
    else if ((text <= 'F') && (text >= 'A'))
    {
        result = (((char) text) - ((char) 'A')) + 10;
    }
    return result;
}

static uint8_t SetWEPKey(const char *password)
{
    int keyLength = strlen(password);
    DRV_WIFI_WEP_CONTEXT context;
    if (keyLength == WEP104_KEY_LENGTH)
    {
        context.wepSecurityType = DRV_WIFI_SECURITY_WEP_104;
        context.wepKeyLength = 13;
    }
    else
    {
        context.wepSecurityType = DRV_WIFI_SECURITY_WEP_40;
        context.wepKeyLength = 5;
    }
    int wepKeyIndex = 0;
    int keyCharIndex = 0;
    for (keyCharIndex = 0; keyCharIndex < keyLength; keyCharIndex += 2, wepKeyIndex++)
    {
        uint8_t byteValue = ((GetHexValue(password[keyCharIndex]) << 4) + GetHexValue(password[keyCharIndex + 1]));
        context.wepKey[wepKeyIndex] = byteValue; // it contains 4 keys make all the same
        context.wepKey[wepKeyIndex + context.wepKeyLength] = byteValue;
        context.wepKey[wepKeyIndex + (context.wepKeyLength * 2)] = byteValue;
        context.wepKey[wepKeyIndex + (context.wepKeyLength * 3)] = byteValue;
    }
    context.wepKeyType = DRV_WIFI_SECURITY_WEP_OPENKEY;
    DRV_WIFI_SecurityWepSet(&context);
    return context.wepSecurityType;
}

bool AppConfig_CheckValidAppConfig(bool readConfigFirst)
{
    bool result = false;
    // TODO - need ConfigStore_DeviceServerConfig_Read()? - acceptable overheads if cert = 16KB?

    if (!readConfigFirst || (ConfigStore_Config_Read() && ConfigStore_Config_IsValid() &&
        ConfigStore_DeviceServerConfig_Read() && ConfigStore_DeviceServerConfig_IsValid()))
    {
        char* bootUrl = (char*) ConfigStore_GetBootstrapURL();
        char* wifiSsid = (char *) ConfigStore_GetNetworkSSID();
        char* wifiPassword = (char *) ConfigStore_GetNetworkPassword();

        // TODO - check extra params for valid device server config
        if ((bootUrl && strlen(bootUrl) > 0) && (wifiSsid && strlen(wifiSsid) > 0) && (wifiPassword && strlen(wifiPassword) > 0))
        {
            result = true;
        }
    }
    return result;
}

bool AppConfig_InitAppCoreTasks(void)
{
    /* Create OS Thread for UI Task. */
    xTaskCreate((TaskFunction_t) AppConfig_UI_Task, "UI Task", 1024, NULL, 1, NULL);

    return true;
}

void AppConfig_Tasks(void)
{
    if (ConfigStore_GetStartInConfigurationMode())
    {
        DRV_WIFI_MGMT_INDICATE_SOFT_AP_EVENT *event = DRV_WIFI_SoftApEventInfoGet();
        if (event)
        {
            if (lastState != event->event)
            {
                if (event->event == DRV_WIFI_SOFTAP_EVENT_CONNECTED)
                {
                    UIControl_SetUIState(AppUIState_SoftApConnected);
                    UIControl_SetUIState(AppUIState_SoftApNotConfigured);
                    CreatorConsole_Puts("\r\n");
                }
                else if (event->event == DRV_WIFI_SOFTAP_EVENT_DISCONNECTED)
                {
                    UIControl_SetUIState(AppUIState_SoftApNotConnected);
                    CreatorConsole_Puts("\r\n");
                }
                lastState = event->event;
            }
        }
    }
    CreatorCommand_Task();
}

static void AppConfig_UI_Task(void)
{
    uint32 currentTickCount = 0;
    uint32 startTick = 0;
    uint32 lastUpTimeUpdate = 0;
    uint32 lastUpTime = SYS_TMR_TickCountGet();
    while (1)
    {
        currentTickCount = SYS_TMR_TickCountGet();

        if (currentTickCount - lastUpTimeUpdate >= SYS_TMR_TickCounterFrequencyGet())
        {
            _UptimeSecs++;
            lastUpTimeUpdate = currentTickCount;
        }

        if (currentTickCount - startTick >= SYS_TMR_TickCounterFrequencyGet() / 2)
        {
            startTick = currentTickCount;
            UIControl_UIStep();
        }

#ifdef SYS_UI_PRINT_UP_TIME
        if (currentTickCount - lastUpTime >= SYS_UI_PRINT_UP_TIME)
        {
            SYS_UPTIME uptime;
            AppConfig_Uptime(&uptime);
            lastUpTime = currentTickCount;
            CreatorConsole_Printf("\tSYS_UI Uptime = %d days, %d hours, %d min, %d seconds\r\n", uptime.Days, uptime.Hours, uptime.Minutes, uptime.Seconds);
        }
#endif
        CreatorThread_SleepMilliseconds(NULL, 100);
    }
}

void AppConfig_Uptime(SYS_UPTIME* uptime)
{
    if (uptime)
    {
        /* Compute uptime components*/
#define	SECS_PER_DAY			(86400)
#define	SECS_PER_HOUR			(3600)
#define	SECS_PER_MINUTE			(60)
        uint32_t uptimeSecs = _UptimeSecs;
        uptime->Days = uptimeSecs / SECS_PER_DAY;
        uptime->Hours = (uptimeSecs - (uptime->Days * SECS_PER_DAY)) / SECS_PER_HOUR;
        uptime->Minutes = (uptimeSecs - (uptime->Days * SECS_PER_DAY) - (uptime->Hours * SECS_PER_HOUR)) / SECS_PER_MINUTE;
        uptime->Seconds = uptimeSecs - (uptime->Days * SECS_PER_DAY) - (uptime->Hours * SECS_PER_HOUR) - (uptime->Minutes * SECS_PER_MINUTE);
    }
}

void AppConfig_InitCreatorCommandHandler(void)
{
    CreatorCommand_Init();

    CreatorCommand_RegisterCommand(NULL, "factory_reset", "Factory reset this device", StandardCommands_FactoryReset);
    CreatorCommand_RegisterCommand(NULL, "reboot", "Reboot device", StandardCommands_Reboot);
    CreatorCommand_RegisterCommand(NULL, "reboot_app", "Reboot device into application mode", StandardCommands_RebootApplicationMode);
    CreatorCommand_RegisterCommand(NULL, "reboot_softap", "Reboot device into SoftAP mode", StandardCommands_RebootSoftAP);
    CreatorCommand_RegisterCommand(NULL, "show", "Display the current value of a device setting", StandardCommands_Show);
    CreatorCommand_RegisterCommand(NULL, "set", "Set the value of a device setting", StandardCommands_Set);
    CreatorCommand_RegisterCommand(NULL, "uptime", "Display the time the device has been running since last reset", StandardCommands_Uptime);

    CreatorCommand_RegisterCommand(NULL, "board_details", "Display board information for output to label printing software", StandardCommands_BoardDetails);
}

void AppConfig_SoftwareReset(bool resetToConfigurationMode)
{
    // Change boot-flag setting if required
    if (resetToConfigurationMode)
    {
        SYS_ASSERT(ConfigStore_Config_Read(), "ERROR: Could not read config_store memory.");

        if (ConfigStore_Config_IsValid())
        {
            // Push new setting to NVM - TODO clean up - use std routine
            ConfigStore_SetResetToConfigurationMode(true);

            if (ConfigStore_Config_UpdateCheckbyte() && ConfigStore_Config_IsValid())
                ConfigStore_Config_Write();
        }
    }
    SYS_RESET_SoftwareReset();

}
