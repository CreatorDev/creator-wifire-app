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

#ifdef POSIX

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdbool.h>

#ifndef CREATOR_DEBUG_ON
#define CREATOR_DEBUG_ON
#endif
#include "creator/core/servertime.h"
#include "creator/core/creator_debug.h"

bool CreatorLog_Initialise()
{
	return true;
}


void Creator_Log(CreatorLogLevel level, const char *message, ...)
{
	va_list vl;
	va_start(vl, message);
	Creator_Logv(level, message, vl);
	va_end(vl);
}

void Creator_Logv(CreatorLogLevel level, const char *message, va_list vl)
{
	FILE * out = stderr;
	char *messageType = "WHAT?: ";
	switch (level)
	{
		case CreatorLogLevel_Debug:
			messageType = "DEBUG: ";
			out = stdout;
			break;
		case CreatorLogLevel_Info:
			messageType = "INFO : ";
			out = stdout;
			break;
		case CreatorLogLevel_Warning:
			messageType = "WARN : ";
			out = stderr;
			break;
		case CreatorLogLevel_Error:
			messageType = "ERROR: ";
			out = stderr;
			break;
		case CreatorLogLevel_None:
			break;
	}

	char time[128];
	time_t t;
	struct tm *tmTime;

	t = CreatorServerTime_GetServerTime();
	tmTime = localtime(&t);
	if (tmTime == NULL)
	{
		time[0] = '\0';
	}
	else if (strftime(time, sizeof(time), "%d/%m/%Y %H:%M:%S", tmTime) == 0)
	{
		time[0] = '\0';
	}

	fprintf(out, "%s, %s", time, messageType);
	vfprintf(out, message, vl);
	fprintf(out, "\n");
}

void CreatorLog_SetLevel(CreatorLogLevel level)
{
}

void CreatorLog_Shutdown()
{
}

#endif
