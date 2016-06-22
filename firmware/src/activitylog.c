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


#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_time.h"
#include "creator/core/creator_debug.h"
#include "creator/creator_console.h"
#include "config_store.h"
#include "activitylog.h"

static void Creator_ActivityLogWriteVargs(CreatorActivityLogLevel level, CreatorActivityLogCategory category, int errorCode, char *szMessage, va_list vl);

static CreatorActivityLogCategory _SystemLoggingMode = CreatorActivityLogCategory_Startup;

void Creator_ActivityLogSystemMode(CreatorActivityLogCategory category)
{
	if (category == CreatorActivityLogCategory_Startup || category == CreatorActivityLogCategory_SystemRuntime)
		_SystemLoggingMode = category;
}

void Creator_ActivityLogWrite(CreatorActivityLogLevel level, CreatorActivityLogCategory category, int errorCode, char *szMessage, ...)
{
    // TODO - replace with Creator_log...
	va_list vl;
	va_start(vl, szMessage);
	Creator_ActivityLogWriteVargs(level, category, errorCode, szMessage, vl);
	va_end(vl);
}

void Creator_ActivityLogWriteVargs(CreatorActivityLogLevel level, CreatorActivityLogCategory category, int errorCode, char *szMessage, va_list vl)
{
	bool writeLogEntry = false;
	bool showDebug = false;

	if (ConfigStore_GetLoggingEnabled())
	{
		CreatorActivityLogLevel loggingLevel = ConfigStore_GetLoggingLevel();
		uint16 loggingCategories = ConfigStore_GetLoggingCategories();
		if (level <= loggingLevel)
		{
			showDebug = true;

			// Map sysRuntime to startup during system startup mode
			if (category == CreatorActivityLogCategory_SystemRuntime)
				category = _SystemLoggingMode;
			if ((1 << (uint16) category) & loggingCategories)
				writeLogEntry = true;
		}
	}

	if (showDebug)
	{
		ActivityLog log;
		memset(log.Message, 0, ACTIVITYLOG_MESSAGE_LENGTH);
		vsnprintf(log.Message, ACTIVITYLOG_MESSAGE_LENGTH, szMessage, vl);
		CreatorConsole_Puts(log.Message);
		CreatorConsole_Puts("\r\n");
	}
}
