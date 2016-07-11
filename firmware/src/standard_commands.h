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

#ifndef CREATOR_COMMAND_HANDLER_H_
#define CREATOR_COMMAND_HANDLER_H_
#ifdef __cplusplus
extern "C"
{
#endif

   

void CreatorCommandCleanup(void);                                      // Handler for resources clean-up
bool StandardCommands_ClearActivityLog(int argc, char** argv);    // Handler to clear_log
bool StandardCommands_FactoryReset(int argc, char** argv);        // Handler to factory_reset
bool StandardCommands_Reboot(int argc, char** argv);              // Handler to reboot

bool StandardCommands_RebootApplicationMode(int argc, char** argv); // Handler to reboot into application mode
bool StandardCommands_RebootSoftAP(int argc, char** argv);        // Handler to reboot into softAP (config) mode

bool StandardCommands_Set(int argc, char** argv);                 // Handler for set
bool StandardCommands_Show(int argc, char** argv);                // HAndler for show
bool StandardCommands_Uptime(int argc, char** argv);              // Handler for uptime

bool StandardCommands_BoardDetails(int argc, char** argv);
bool StandardCommands_DisplayBoardDetails(int argc, char** argv);

int StandardCommands_ResetHandler(bool resetToConfigurationMode);
bool StandardCommands_SaveNewValue(int argc, char** argv);
bool StandardCommands_SendAsyncMessageCommand(int argc, char** argv);
bool StandardCommands_UpdateOwnedDevices(int argc, char** argv);



/*Get Command Handlers*/
void StandardCommands_GetVersions(void);             // Handler to get Versions
void StandardCommands_GetDeviceName(void);           // Handler to get Devicename
void StandardCommands_GetDevRegKey(void);            // Handler to get Device Registration key
void StandardCommands_GetCreatorConfig(void);           // Handler to get CreatorConfiguration
void StandardCommands_GetLogConfig(void);            // Handler to get LogConfiguration
void StandardCommands_GetNetworkConfig(void);        // Handler to get NetworkConfiguration
void StandardCommands_GetActivityLog(void);          // Handler to get ActivityLogs

/*Set Command Handlers*/
void StandardCommands_SetDeviceName(void);           // Handler to Set DeviceName
void StandardCommands_SetDevRegKey(void);            // Handler to set Device Registration Key
void StandardCommands_SetCreatorConfig(void);           // Handler to set CreatorConfiguration
void StandardCommands_SetLogConfig(void);            // Handler to set LogConfiguration

bool StandardCommands_FactoryResetHelper(void);

#ifdef __cplusplus
}
#endif

#endif /* CREATOR_COMMAND_HANDLER_H_ */

