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

#ifndef _APP_CONFIG_H
#define _APP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif
	
#include <stdint.h>
#include "creator/core/creator_threading.h"
#include "creator_command.h"
	
typedef bool (*AppBase_ResetHandler)(bool resetToConfigurationMode);

typedef struct
{
	char *ApplicationName;
	char *ApplicationVersion;
	char *ApplicationVersionDate;	

	CreatorThread_Callback AppTask;

	CreatorCommand_Handler CommandSet;
	CreatorCommand_Handler CommandShow;
	AppBase_ResetHandler AppCLI_ResetHandler;
} AppInfo;
	
typedef struct
{
    /* Uptime components*/
    uint16_t Days;
	uint8_t Hours;
	uint8_t	Minutes;
	uint8_t	Seconds;
} SYS_UPTIME;


bool AppConfig_CheckForConfigurationModeRebootButtonPress(void);

bool AppConfig_CheckValidAppConfig(bool readConfigFirst);

void AppConfig_StartConfigWebServer(uint8 *serverCert, int serverCertLength);

void AppConfig_CreatorInitialise(void);

AppInfo *AppConfig_GetAppInfo(void);

char *AppConfig_GetVersion(void);

char *AppConfig_GetVersionDate(void);

void AppConfig_Initialise(AppInfo *info);

bool AppConfig_IsDeviceOnline(void);

bool AppConfig_IsForcePresencePublish(void);

bool AppConfig_IsRunningInConfigurationMode(void); // Check if App is in configuration (SoftAP) or application mode	

void AppConfig_NetworkInitialise(void);

void AppConfig_SetDeviceOnline(bool deviceStatus);
void AppConfig_SetForcePresencePublish(bool forcePublish);

bool AppConfig_SetResetMode(bool configMode);

void AppConfig_SoftwareReset(bool resetToConfigurationMode);

void AppConfig_Tasks(void);

void AppConfig_Uptime (SYS_UPTIME* uptime);



#ifdef __cplusplus
}
#endif

#endif /* _APP_CONFIG_H */
