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

#include "creator/core/core.h"
#include "creator/core/http_server.h"
#include "creator/core/creator_task_scheduler.h"
#include "creator/core/xmltree.h"

#include "app_config.h"
#include "string_builder.h"
#include "config_store.h"
#include "ui_control.h"

#include "system_config.h"

#include "system/debug/sys_debug.h"


#define HTTP_CONTENTTYPE_CREATOR_DEVICEINFO	"application/xml; application/vnd.imgtec.com.device-info+xml; charset=utf-8"
#define HTTP_CONTENTTYPE_CREATOR_DEVICESERVER	"application/xml; application/vnd.imgtec.com.device-server+xml; charset=utf-8"
#define HTTP_CONTENTTYPE_CREATOR_NETWORKCONFIG	"application/xml; application/vnd.imgtec.com.network-config+xml; charset=utf-8"

#define DEFAULT_ACTIVITYLOG_PAGESIZE	(5)
#define MAX_ACTIVITYLOG_PAGESIZE		(5)


typedef void (*RequestHandler)(CreatorHTTPServerRequest request);

static void GetDeviceInfo(CreatorHTTPServerRequest request);
static void GetDeviceName(CreatorHTTPServerRequest request);
static void GetDeviceServer(CreatorHTTPServerRequest request);
static void GetNetworkConfig(CreatorHTTPServerRequest request);
static void GetActivityLog(CreatorHTTPServerRequest request);
static void GetNotImplemented(CreatorHTTPServerRequest request);

static void PostNotImplemented(CreatorHTTPServerRequest request);
static void PostDeviceName(CreatorHTTPServerRequest request);
static void PostDeviceServer(CreatorHTTPServerRequest request);
static void PostNetworkConfig(CreatorHTTPServerRequest request);
static void PostReset(CreatorHTTPServerRequest request);
static void PostResetToSoftAP(CreatorHTTPServerRequest request);
static void PostFactoryReset(CreatorHTTPServerRequest request);

typedef struct
{
	uint8_t*		name;
	uint8_t*		endpoint;
	RequestHandler	getHandler;
	RequestHandler	postHandler;
	bool			checkForPostBody;
} wsEndpointMap;

// Mapping between endpoints and handler functions
const wsEndpointMap wsAllEndpoints[] =
{
	{(uint8_t*) "Device Info",		(uint8_t*) "device",            GetDeviceInfo,		PostNotImplemented,	true},
	{(uint8_t*) "Device Name",		(uint8_t*) "name",              GetDeviceName,		PostDeviceName,		true},
	{(uint8_t*) "Device Server",	(uint8_t*) "deviceserver",      GetDeviceServer,	PostDeviceServer,	true},
	{(uint8_t*) "Network Config",	(uint8_t*) "network",           GetNetworkConfig,	PostNetworkConfig,	true},
	{(uint8_t*) "Reboot",			(uint8_t*) "reboot",			GetNotImplemented,	PostReset,			false},
	{(uint8_t*) "Reboot to SoftAP",	(uint8_t*) "rebootsoftap",      GetNotImplemented,	PostResetToSoftAP,	false},
	{(uint8_t*) "Factory Reset",	(uint8_t*) "factoryreset",		GetNotImplemented,	PostFactoryReset,	false}
};
#define NUM_ENDPOINTS	(sizeof(wsAllEndpoints)/sizeof(wsEndpointMap))


static void GetDeviceInfo(CreatorHTTPServerRequest request)
{
	if (request)
	{
		// Read device configuration
		if (ConfigStore_Config_Read() && ConfigStore_Config_IsValid())
		{
			StringBuilder responseBody = StringBuilder_New(256);
			if (responseBody)
			{
				//
				// Build response string
				//

				// Start of response
				responseBody = StringBuilder_Append(responseBody,  "<DeviceInfo>");

				responseBody = StringBuilder_Append(responseBody,  "<DeviceName>");
				responseBody = StringBuilder_Append(responseBody,  ConfigStore_GetDeviceName());
				responseBody = StringBuilder_Append(responseBody,  "</DeviceName>");

   				responseBody = StringBuilder_Append(responseBody,  "<ClientID>");
				// TODO - get clientID (if any)
				responseBody = StringBuilder_Append(responseBody,  "</ClientID>");

				responseBody = StringBuilder_Append(responseBody,  "<DeviceType>");
				responseBody = StringBuilder_Append(responseBody,  ConfigStore_GetDeviceType());
				responseBody = StringBuilder_Append(responseBody,  "</DeviceType>");

				responseBody = StringBuilder_Append(responseBody,  "<SerialNumber>");
				{
					uint8_t snBuff[17];
					if (DeviceSerial_GetCpuSerialNumberHexString((char *) snBuff, 17))
						responseBody = StringBuilder_Append(responseBody, (const char *) snBuff);
				}
				responseBody = StringBuilder_Append(responseBody,  "</SerialNumber>");

				responseBody = StringBuilder_Append(responseBody,  "<MACAddress>");
				responseBody = StringBuilder_Append(responseBody,  ConfigStore_GetMacAddress());
				responseBody = StringBuilder_Append(responseBody,  "</MACAddress>");

				responseBody = StringBuilder_Append(responseBody,  "<SoftwareVersion>");
				AppInfo *appInfo = AppConfig_GetAppInfo();
				if (appInfo)
					responseBody = StringBuilder_Append(responseBody,   appInfo->ApplicationVersion);
				else
					responseBody = StringBuilder_Append(responseBody,   "UNKNOWN");
				responseBody = StringBuilder_Append(responseBody,  "</SoftwareVersion>");

				// End of response
				responseBody = StringBuilder_Append(responseBody,  "</DeviceInfo>");

				uint32_t responseBodyLen = StringBuilder_GetLength(responseBody);
				CreatorHTTPServerRequest_SendResponse(request, CreatorHTTPStatus_OK, HTTP_CONTENTTYPE_CREATOR_DEVICEINFO,  (void *)StringBuilder_GetCString(responseBody), responseBodyLen, true);	 
				StringBuilder_Free(&responseBody);
			}
		}
	}
}

static void GetDeviceName(CreatorHTTPServerRequest request)
{
	if (request)
	{
		// Read device configuration
		if (ConfigStore_Config_Read() && ConfigStore_Config_IsValid())
		{
			StringBuilder responseBody = StringBuilder_New(256);
			if (responseBody)
			{
				//
				// Build response string
				//

				// Start of response
				responseBody = StringBuilder_Append(responseBody,  "<DeviceName>");

				// Device Name //
				responseBody = StringBuilder_Append(responseBody,  "<Name>");
				responseBody = StringBuilder_Append(responseBody,  ConfigStore_GetDeviceName());
				responseBody = StringBuilder_Append(responseBody,  "</Name>");

				// End of response
				responseBody = StringBuilder_Append(responseBody,  "</DeviceName>");

				uint32_t responseBodyLen = StringBuilder_GetLength(responseBody);
				CreatorHTTPServerRequest_SendResponse(request, CreatorHTTPStatus_OK, HTTP_CONTENTTYPE_CREATOR_DEVICEINFO,  (void *)StringBuilder_GetCString(responseBody), responseBodyLen, true);	 
				StringBuilder_Free(&responseBody);
			}
		}
	}
}

// Device server configuration
static void GetDeviceServer(CreatorHTTPServerRequest request)
{
	if (request)
	{
		// Read device server configuration
		if (ConfigStore_DeviceServerConfig_Read() && ConfigStore_DeviceServerConfig_IsValid())
		{
			StringBuilder responseBody = StringBuilder_New(256);
			if (responseBody)
			{
				//
				// Build response string
				//

				// Start of response
				responseBody = StringBuilder_Append(responseBody,  "<DeviceServer>");

				// WiFi SSID
				responseBody = StringBuilder_Append(responseBody,  "<BootstrapUrl>");
				responseBody = StringBuilder_Append(responseBody,  ConfigStore_GetBootstrapURL());
				responseBody = StringBuilder_Append(responseBody,  "</BootstrapUrl>");

				responseBody = StringBuilder_Append(responseBody,  "<SecurityMode>");
				switch(ConfigStore_GetSecurityMode())
				{
					case ServerSecurityMode_NoSec:
						responseBody = StringBuilder_Append(responseBody,  "NoSec");
						break;
					case ServerSecurityMode_PSK:
						responseBody = StringBuilder_Append(responseBody,  "PSK");
						break;
					case ServerSecurityMode_Cert:
						responseBody = StringBuilder_Append(responseBody,  "Cert");
						break;
					default:
						break;
				}
				responseBody = StringBuilder_Append(responseBody,  "</SecurityMode>");

                if (ConfigStore_GetSecurityMode() == ServerSecurityMode_PSK)
                {
                    responseBody = StringBuilder_Append(responseBody,  "<PublicKey>");
                    responseBody = StringBuilder_Append(responseBody,  ConfigStore_GetPublicKey());
                    responseBody = StringBuilder_Append(responseBody,  "</PublicKey>");
                }

				// End of response
				responseBody = StringBuilder_Append(responseBody,  "</DeviceServer>");

				uint32_t responseBodyLen = StringBuilder_GetLength(responseBody);
				CreatorHTTPServerRequest_SendResponse(request, CreatorHTTPStatus_OK, HTTP_CONTENTTYPE_CREATOR_NETWORKCONFIG,  (void *)StringBuilder_GetCString(responseBody), responseBodyLen, true);	 
				StringBuilder_Free(&responseBody);
			}
		}
	}
}

// WiFi network configuration
static void GetNetworkConfig(CreatorHTTPServerRequest request)
{
	if (request)
	{
		// Read device configuration
		if (ConfigStore_Config_Read() && ConfigStore_Config_IsValid())
		{
			StringBuilder responseBody = StringBuilder_New(256);
			if (responseBody)
			{
				//
				// Build response string
				//

				// Start of response
				responseBody = StringBuilder_Append(responseBody,  "<NetworkConfig>");

				// WiFi SSID
				responseBody = StringBuilder_Append(responseBody,  "<SSID>");
				responseBody = StringBuilder_Append(responseBody,  ConfigStore_GetNetworkSSID());
				responseBody = StringBuilder_Append(responseBody,  "</SSID>");

				// Encryption Details
				responseBody = StringBuilder_Append(responseBody,  "<Encryption>");
				switch(ConfigStore_GetEncryptionType())
				{
					case WiFiEncryptionType_WEP:
						responseBody = StringBuilder_Append(responseBody,  "WEP");
						break;
					case WiFiEncryptionType_WPA:
						responseBody = StringBuilder_Append(responseBody,  "WPA");
						break;
					case WiFiEncryptionType_WPA2:
						responseBody = StringBuilder_Append(responseBody,  "WPA2");
						break;
					case WiFiEncryptionType_Open:
						responseBody = StringBuilder_Append(responseBody,  "Open");
						break;
					default:
						break;
				}
				responseBody = StringBuilder_Append(responseBody,  "</Encryption>");

				responseBody = StringBuilder_Append(responseBody,  "<Password></Password>");

				// Addressing scheme
				responseBody = StringBuilder_Append(responseBody,  "<AddrMethod>");
				switch(ConfigStore_GetAddressingScheme())
				{
					case AddressScheme_StaticIP:
						responseBody = StringBuilder_Append(responseBody,  "static");
						break;
					case AddressScheme_Dhcp:
						responseBody = StringBuilder_Append(responseBody,  "dhcp");
						break;
					default:
						break;
				}
				responseBody = StringBuilder_Append(responseBody,  "</AddrMethod>");

				// Static networking settings
				responseBody = StringBuilder_Append(responseBody,  "<StaticDNS>");
				responseBody = StringBuilder_Append(responseBody,  ConfigStore_GetStaticDNS());
				responseBody = StringBuilder_Append(responseBody,  "</StaticDNS>");

				responseBody = StringBuilder_Append(responseBody,  "<StaticIP>");
				responseBody = StringBuilder_Append(responseBody,  ConfigStore_GetStaticIP());
				responseBody = StringBuilder_Append(responseBody,  "</StaticIP>");

				responseBody = StringBuilder_Append(responseBody,  "<StaticNetmask>");
				responseBody = StringBuilder_Append(responseBody,  ConfigStore_GetStaticNetmask());
				responseBody = StringBuilder_Append(responseBody,  "</StaticNetmask>");

				responseBody = StringBuilder_Append(responseBody,  "<StaticGateway>");
				responseBody = StringBuilder_Append(responseBody,  ConfigStore_GetStaticGateway());
				responseBody = StringBuilder_Append(responseBody,  "</StaticGateway>");

				// End of response
				responseBody = StringBuilder_Append(responseBody,  "</NetworkConfig>");

				uint32_t responseBodyLen = StringBuilder_GetLength(responseBody);
				CreatorHTTPServerRequest_SendResponse(request, CreatorHTTPStatus_OK, HTTP_CONTENTTYPE_CREATOR_NETWORKCONFIG,  (void *)StringBuilder_GetCString(responseBody), responseBodyLen, true);	 
				StringBuilder_Free(&responseBody);
			}
		}
	}
}


static void GetNotImplemented(CreatorHTTPServerRequest request)
{
	CreatorHTTPServerRequest_SendResponse(request, CreatorHTTPStatus_NotImplemented, NULL,  NULL, 0, true);	 	
}

static void PostNotImplemented(CreatorHTTPServerRequest request)
{
	CreatorHTTPServerRequest_SendResponse(request, CreatorHTTPStatus_NotImplemented, NULL,  NULL, 0, true);	 	
}

static void PostDeviceName(CreatorHTTPServerRequest request)
{
	SYS_ASSERT(ConfigStore_Config_Read(), "ERROR: Could not read config_store memory.");

	if (request && ConfigStore_Config_IsValid())
	{
		// Set up parser to parse the POST body
		TreeNode xmlTreeRoot = TreeNode_ParseXML((uint8_t*) CreatorHTTPServerRequest_GetContent(request), CreatorHTTPServerRequest_GetContentLength(request), true);
		if (xmlTreeRoot)
		{
			// Extract response data and save config
			TreeNode node = NULL;
			node = TreeNode_Navigate(xmlTreeRoot, "DeviceName/Name");
			if (node)
				ConfigStore_SetDeviceName((const char *) TreeNode_GetValue(node));
		}

		// Fell the tree
		Tree_Delete(xmlTreeRoot);
		xmlTreeRoot = NULL;

		// Push new config to NVM
		if ( ConfigStore_Config_UpdateCheckbyte() && ConfigStore_Config_IsValid())
			ConfigStore_Config_Write();
		
		CreatorHTTPServerRequest_SendResponse(request, CreatorHTTPStatus_NoContent, NULL,  NULL, 0, true);	 
	}
}

static void PostFactoryReset(CreatorHTTPServerRequest request)
{
	if (request)
	{
		int statusCode = CreatorHTTPStatus_NoContent;
		// Preserve existing SoftAP password
		char *existingSoftAPPassword = NULL;
		const char* softApPassword = ConfigStore_GetSoftAPPassword();
		uint8 softApPasswordLength = strlen(softApPassword);

		existingSoftAPPassword = (char*) Creator_MemAlloc((softApPasswordLength+1) * sizeof(char));		
		if (existingSoftAPPassword)
		{
			memset(existingSoftAPPassword, '\0', (softApPasswordLength+1) * sizeof(char) );
			memcpy(existingSoftAPPassword, softApPassword, softApPasswordLength);
		}

		if (StandardCommands_FactoryResetHelper())
		{
			if (existingSoftAPPassword)
			{
				if (!ConfigStore_SetSoftAPPassword(existingSoftAPPassword) || !ConfigStore_Config_UpdateCheckbyte() || !ConfigStore_Config_Write())
					statusCode = CreatorHTTPStatus_InternalServerError;

				Creator_MemFree((void **) &existingSoftAPPassword);
			}
		}
		else
			statusCode = CreatorHTTPStatus_InternalServerError;
		CreatorHTTPServerRequest_SendResponse(request, statusCode, NULL,  NULL, 0, true);	 
	}
}

static void PostDeviceServer(CreatorHTTPServerRequest request)
{
	SYS_ASSERT(ConfigStore_DeviceServerConfig_Read(), "ERROR: Could not read device server config_store memory.");
	if (request && ConfigStore_DeviceServerConfig_IsValid())
	{
		// Set up parser to parse the POST body
		TreeNode xmlTreeRoot = TreeNode_ParseXML((uint8_t*) CreatorHTTPServerRequest_GetContent(request), CreatorHTTPServerRequest_GetContentLength(request), true);
		if (xmlTreeRoot)
		{
			// Extract response data and save config
			TreeNode node = NULL;
            
			node = TreeNode_Navigate(xmlTreeRoot, "DeviceServer/BootstrapUrl");
			if (node) {
				ConfigStore_SetBootstrapURL((const char *) TreeNode_GetValue(node));
            }
			node = TreeNode_Navigate(xmlTreeRoot, "DeviceServer/SecurityMode");
			if (node)
			{
				char *value = (char*) TreeNode_GetValue(node);
				if (value)
				{
                    // TODO - case indep support...
                    // TODO - wipe other settings first - if not set
					if (strcmp(value, "NoSec") == 0)
						ConfigStore_SetSecurityMode(ServerSecurityMode_NoSec);
					else if (strcmp(value, "PSK") == 0)
						ConfigStore_SetSecurityMode(ServerSecurityMode_PSK);
					else if (strcmp(value, "Cert") == 0)
						ConfigStore_SetSecurityMode(ServerSecurityMode_Cert);
				}
			}

            ServerSecurityMode securityMode = ConfigStore_GetSecurityMode();
            if (securityMode == ServerSecurityMode_PSK)
            {
                node = TreeNode_Navigate(xmlTreeRoot, "DeviceServer/PublicKey");
                if (node)
                    ConfigStore_SetPublicKey((const char *) TreeNode_GetValue(node));

                node = TreeNode_Navigate(xmlTreeRoot, "DeviceServer/PrivateKey");
                if (node)
                    ConfigStore_SetPrivateKey((const char *) TreeNode_GetValue(node));
            }
            else
            {
                ConfigStore_SetPublicKey("");
                ConfigStore_SetPrivateKey("");
            }

            if (securityMode == ServerSecurityMode_Cert)
            {
                node = TreeNode_Navigate(xmlTreeRoot, "DeviceServer/Certificate");
                if (node)
                    ConfigStore_SetCertificate((const char *) TreeNode_GetValue(node));

                node = TreeNode_Navigate(xmlTreeRoot, "DeviceServer/BootstrapCertChain");
                if (node)
                    ConfigStore_SetBootstrapCertChain((const char *) TreeNode_GetValue(node));
            }
            else
            {
                ConfigStore_SetCertificate("");
                ConfigStore_SetBootstrapCertChain("");
            }

			Creator_ActivityLogWrite(CreatorActivityLogLevel_Information, CreatorActivityLogCategory_Startup, CREATOR_ACTIVITY_ERRORCODE_NONE,
					"SoftAP device server configured: Security = %s, Bootstrap = %s", ConfigStore_GetSecurityModeName(securityMode), ConfigStore_GetBootstrapURL());
            // TODO - try to extract/show clientID from cert or PSK

		}
		// Fell the tree
		Tree_Delete(xmlTreeRoot);
		xmlTreeRoot = NULL;

		// Push new config to NVM
		if (ConfigStore_DeviceServerConfig_UpdateCheckbyte() && ConfigStore_DeviceServerConfig_IsValid())
			ConfigStore_DeviceServerConfig_Write();
        
        if (AppConfig_CheckValidAppConfig(false))
		{
   			// Next boot should be into application mode
			UIControl_SetUIState(AppUIState_SoftApConfigured);
			ConfigStore_SetResetToConfigurationMode(false);
    		if (ConfigStore_Config_UpdateCheckbyte() && ConfigStore_Config_IsValid())
        		ConfigStore_Config_Write();
        }
		CreatorHTTPServerRequest_SendResponse(request, CreatorHTTPStatus_NoContent, NULL,  NULL, 0, true);
	}
}

// WiFi network configuration
static void PostNetworkConfig(CreatorHTTPServerRequest request)
{
	SYS_ASSERT(ConfigStore_Config_Read(), "ERROR: Could not read config_store memory.");
	if (request && ConfigStore_Config_IsValid())
	{
		// Set up parser to parse the POST body
		TreeNode xmlTreeRoot = TreeNode_ParseXML((uint8_t*) CreatorHTTPServerRequest_GetContent(request), CreatorHTTPServerRequest_GetContentLength(request), true);
		if (xmlTreeRoot)
		{
			// Extract response data and save config
			TreeNode node = NULL;
            
			node = TreeNode_Navigate(xmlTreeRoot, "NetworkConfig/SSID");
			if (node) {
				ConfigStore_SetNetworkSSID((const char *) TreeNode_GetValue(node));
            }
			node = TreeNode_Navigate(xmlTreeRoot, "NetworkConfig/Encryption");
			if (node)
			{
				char *value = (char*) TreeNode_GetValue(node);
				if (value)
				{
					if (strcmp(value, "WEP") == 0)
						ConfigStore_SetNetworkEncryption(WiFiEncryptionType_WEP);
					else if (strcmp(value, "WPA") == 0)
						ConfigStore_SetNetworkEncryption(WiFiEncryptionType_WPA);
					else if (strcmp(value, "WPA2") == 0)
						ConfigStore_SetNetworkEncryption(WiFiEncryptionType_WPA2);
					else if (strcmp(value, "Open") == 0)
						ConfigStore_SetNetworkEncryption(WiFiEncryptionType_Open);
				}
			}

			node = TreeNode_Navigate(xmlTreeRoot, "NetworkConfig/Password");
			if (node) {
				ConfigStore_SetNetworkPassword((const char *) TreeNode_GetValue(node));
			}
			node = TreeNode_Navigate(xmlTreeRoot, "NetworkConfig/AddrMethod");
			if (node)
			{
				uint8_t* value = (uint8_t*) TreeNode_GetValue(node);
				if (value)
				{
					if (strcmp((char*) value, "dhcp") == 0)
						ConfigStore_SetAddressingScheme(AddressScheme_Dhcp);
					else if (strcmp((char*) value, "static") == 0)
						ConfigStore_SetAddressingScheme(AddressScheme_StaticIP);
				}
			}

			node = TreeNode_Navigate(xmlTreeRoot, "NetworkConfig/StaticDNS");
			if (node)
				ConfigStore_SetStaticDNS((const char *) TreeNode_GetValue(node));

			node = TreeNode_Navigate(xmlTreeRoot, "NetworkConfig/StaticIP");
			if (node)
				ConfigStore_SetStaticIP((const char *) TreeNode_GetValue(node));

			node = TreeNode_Navigate(xmlTreeRoot, "NetworkConfig/StaticNetmask");
			if (node)
				ConfigStore_SetStaticNetmask((const char *) TreeNode_GetValue(node));

			node = TreeNode_Navigate(xmlTreeRoot, "NetworkConfig/StaticGateway");
			if (node)
				ConfigStore_SetStaticGateway((const char *) TreeNode_GetValue(node));

    		const char *security = "";
			switch(ConfigStore_GetEncryptionType())
			{
				case WiFiEncryptionType_WEP:
					security = "WEP";
					break;

				case WiFiEncryptionType_WPA:
					security = "WPA";
					break;

				case WiFiEncryptionType_WPA2:
					security = "WPA2";
					break;

				case WiFiEncryptionType_Open:
					security = "Open";
					break;

				default:
					security = "WPA2";
					break;
			}
			Creator_ActivityLogWrite(CreatorActivityLogLevel_Information, CreatorActivityLogCategory_Startup, CREATOR_ACTIVITY_ERRORCODE_NONE,
					"SoftAP device configured: SSID = %s, Security = %s", ConfigStore_GetNetworkSSID(), security);

            ConfigStore_SetNetworkConfigSet(true);
            if (AppConfig_CheckValidAppConfig(false))
            {
                // Next boot should be into application mode
                UIControl_SetUIState(AppUIState_SoftApConfigured);
                ConfigStore_SetResetToConfigurationMode(false);
            }
		}
		// Fell the tree
		Tree_Delete(xmlTreeRoot);
		xmlTreeRoot = NULL;

		// Push new config to NVM
		if (ConfigStore_Config_UpdateCheckbyte() && ConfigStore_Config_IsValid())
			ConfigStore_Config_Write();
		
		CreatorHTTPServerRequest_SendResponse(request, CreatorHTTPStatus_NoContent, NULL,  NULL, 0, true);
	}
}


static void ResetTimeoutTask(CreatorTaskID taskID, void *clientArg)
{
	CreatorConsole_Puts("Restarting the device...");
	AppConfig_SoftwareReset(false);
}


static void PostReset(CreatorHTTPServerRequest request)
{
	if (request)
	{
        if (AppConfig_CheckValidAppConfig(true) && ConfigStore_StartInConfigMode())
		{
   			// Next boot should be into application mode
			UIControl_SetUIState(AppUIState_SoftApConfigured);
			ConfigStore_SetResetToConfigurationMode(false);
    		if (ConfigStore_Config_UpdateCheckbyte() && ConfigStore_Config_IsValid())
        		ConfigStore_Config_Write();
        }
        
        CreatorScheduler_Initialise();
        CreatorScheduler_ScheduleTask(ResetTimeoutTask, NULL, 1, false);  
		//AppConfig_SoftwareReset(false);
		CreatorHTTPServerRequest_SendResponse(request, CreatorHTTPStatus_NoContent, NULL,  NULL, 0, true);	 
	}
}

static void PostResetToSoftAP(CreatorHTTPServerRequest request)
{
	if (request)
	{		
		AppConfig_SoftwareReset(true);
		CreatorHTTPServerRequest_SendResponse(request, CreatorHTTPStatus_NoContent, NULL,  NULL, 0, true);	 
	}
}

static void ProcessRequest(CreatorHTTPServer server, CreatorHTTPServerRequest request)
{
	CreatorHTTPQuery query = CreatorHTTPServerRequest_GetUrl(request);
	if (query)
	{
		CreatorHTTPMethod method = CreatorHTTPServerRequest_GetMethod(request);
#if DEBUG	
		CreatorConsole_Puts("Request ");
		switch(method)
		{
			case CreatorHTTPMethod_Get:
				CreatorConsole_Puts("GET ");
				break;
			case CreatorHTTPMethod_Post:
				CreatorConsole_Puts("POST ");
				break;
			case CreatorHTTPMethod_Put:
				CreatorConsole_Puts("PUT ");
				break;
			case CreatorHTTPMethod_Delete:
				CreatorConsole_Puts("DELETE ");
				break;
		}
#endif		
		char *url =  CreatorHTTPQuery_GetBaseUrl(query);
#if DEBUG		
		CreatorConsole_Puts(url);
		CreatorConsole_Puts("\r\n");
#endif	
		char *dir = strstr(url, "/");
		if (dir == url)
		{
			char *endpoint = dir + 1;
			// Search for endpoint
			uint8_t endpointIndex = 0;
			for (endpointIndex = 0; endpointIndex < NUM_ENDPOINTS; endpointIndex++)
			{
				// Endpoint match
				if (strcmp((const char*) wsAllEndpoints[endpointIndex].endpoint, (const char*) endpoint) == 0)
				{
					CreatorHTTPMethod method = CreatorHTTPServerRequest_GetMethod(request);
					if (method == CreatorHTTPMethod_Get)
					{
						if (wsAllEndpoints[endpointIndex].getHandler)
							wsAllEndpoints[endpointIndex].getHandler(request);
					}
					else if (method == CreatorHTTPMethod_Post)
					{
						if (wsAllEndpoints[endpointIndex].postHandler)
							wsAllEndpoints[endpointIndex].postHandler(request);
					}
					break;
				}
			}
		}
	}
}

void AppConfig_StartConfigWebServer(uint8 *serverCert, int serverCertLength)
{
    CreatorCore_Initialise();
    CreatorHTTPServer_Initialise();
    CreatorHTTPServer server;
    if (serverCert)
    {
        server = CreatorHTTPServer_New(443, true, ProcessRequest);
        CreatorHTTPServer_SetCertificate(server, serverCert, serverCertLength, 0);
    }
    else
    {
        server = CreatorHTTPServer_New(80, false, ProcessRequest);
    }
    if (CreatorHTTPServer_Start(server))
    {
        CreatorConsole_Puts("Http Server started\r\n");
    }
}