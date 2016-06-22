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


//getting strnlen
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#ifdef POSIX
#include <unistd.h>
#endif
#include <string.h>

#include "creator/core/core.h"
#include "creator/core/creator_task_scheduler.h"
#include "creator/core/client_private.h"
//#include "creator/core/xml_serialisation.h"
//#include "creator/core/http_private.h"
#include "creator_http.h"
#include "creator/core/creator_debug.h"
#include "creator/core/creator_nvs.h"
#include "creator/core/creator_time.h"
#include "creator/core/creator_timer.h"
#include "creator/core/common_messaging_main.h"
#include "creator/core/creator_cert_private.h"

//#include "creator_cache.h"
#include "creator_threading_private.h"
#include "creator/core/creator_random_private.h"

#ifndef LIB_VERSION
#define LIB_VERSION developer
#endif

#ifndef LIB_VERSIONDATE
#define LIB_VERSIONDATE date
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

static bool _IsInitialised = false;

char *CreatorCore_GetVersion(void)
{
	return TOSTRING(LIB_VERSION);
}


char *CreatorCore_GetVersionDate(void)
{
	return TOSTRING(LIB_VERSIONDATE);
}


bool CreatorCore_Initialise()
{
	//FIXME this should probably be done in the main, not in our library
#ifdef POSIX
	Creator_SetRandomSeed(Creator_GetTime(NULL)*getpid());
#else
	Creator_SetRandomSeed(Creator_GetTime(NULL));
#endif
	return CreatorCore_InitialiseLibOnly();
}

bool CreatorCore_InitialiseLibOnly()
{
	//NB Creator_Assert can be used here, as it would fail only if it's been initialized
	Creator_Assert(!_IsInitialised, "CreatorCore_Shutdown must be called before CreatorCore_Initialise is called again");
	if (!_IsInitialised)
	{
		_IsInitialised = true;
		bool bSuccess = true;
		bool bLogStarted = true;

		CreatorThread_Initialise();
		CreatorTimer_Initialise();
		bSuccess &= CreatorCert_Initialise();
		bSuccess &= bLogStarted = CreatorLog_Initialise();
		bSuccess &= CreatorNVS_Initialise();
		bSuccess &= CreatorScheduler_Initialise();
		CreatorHTTP_Initialise();

		if (!bSuccess)
		{
			if (bLogStarted)
			{
				Creator_Log(CreatorLogLevel_Error, "libcreator core initialization failed!");
			}
			CreatorCore_Shutdown();
		}

		return bSuccess;
	}
	return false;
}

void CreatorCore_Shutdown()
{
	if (_IsInitialised)
	{
		CreatorScheduler_Shutdown();
		CreatorNVS_Shutdown();
		CreatorLog_Shutdown();
		CreatorHTTP_Shutdown();
		CreatorCert_Shutdown();
		CreatorTimer_Shutdown();
		CreatorThread_Shutdown();
		_IsInitialised = false;
	}
}

