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

/*! \file command_handlers.c
 *  \brief Callback functions for handling HTTP and CLI commands
 */

#include "app.h"
#include "command_handlers.h"

#include "creator/creator_console.h"
#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_task_scheduler.h"
#include "app_config.h"
#include "config_store.h"
#include "device_serial.h"
#include "string_builder.h"
#include "ui_control.h"

#include "arduino_monitor.h"


bool _ResetPending = false;
bool _ResetToConfigurationMode = false;



#ifdef CREATOR_PIC32MZ_ETHERNET

void BSP_VBUSSwitchDisable(void)
{
	/* Disable the VBUS switch */
	ANSELBbits.ANSB5 = 0;
	TRISBbits.TRISB5 = 0;
	LATBbits.LATB5 = 0;
}
#endif

char *CommandHandlers_GetTimeString(void)
{
#define DATETIME_FIELD_LENGTH (21)
	char *datetime = Creator_MemAlloc(sizeof (char) * DATETIME_FIELD_LENGTH);
	if (datetime)
	{
		time_t currentDateTime;
		Creator_GetTime(&currentDateTime);
		struct tm *_currentDateTime = gmtime(&currentDateTime);
		memset(datetime, '\0', DATETIME_FIELD_LENGTH);
		if (strftime((char *) datetime, DATETIME_FIELD_LENGTH, "%Y-%m-%dT%H:%M:%SZ", _currentDateTime) != 0)
		{
			// success
		}
	}
	return datetime;
}

static void ResetTimeoutTask(CreatorTaskID taskID, void *clientArg)
{
	CreatorConsole_Puts("Forcing restart...");
	CreatorThread_SleepMilliseconds(NULL, 200);
	AppConfig_SoftwareReset(_ResetToConfigurationMode);
}

bool CommandHandlers_ResetHandler(bool resetToConfigurationMode)
{
	CreatorScheduler_ScheduleTask(ResetTimeoutTask, NULL, 10, false);

	if (AppConfig_IsDeviceOnline())
	{
		_ResetToConfigurationMode = resetToConfigurationMode;
		_ResetPending = true;
	}
	else
	{
		AppConfig_SoftwareReset(resetToConfigurationMode);
	}
	return true;
}

bool CommandHandlers_IsResetPending(void)
{
	return _ResetPending;
}

bool CommandHandlers_ResetToConfigurationMode(void)
{
	return _ResetToConfigurationMode;
}






