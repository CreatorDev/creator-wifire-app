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

#ifdef FREERTOS

//enable nanosleep
#define _POSIX_C_SOURCE 199506L
#include <stddef.h>
#include <time.h>
#include <errno.h>
#include "FreeRTOS.h"
#include "task.h"

#include "creator/core/creator_timer.h"
#include "creator/core/creator_threading.h"
#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_list.h"
//#include "creator/core/client.h"

#define CREATOR_STACK_SIZE		(4096)		// needed for CMP + TCP overheads (note: configMINIMAL_STACK_SIZE was 2K)

typedef struct
{
	CreatorThread_Callback Runnable;
	void *Context;
	xTaskHandle ThreadID;
} ThreadInfo;

typedef struct
{
	uint ThreadID;
	CreatorErrorType Error;
} ThreadError;

static CreatorList _ThreadErrors = NULL;

typedef struct
{
	uint ThreadID;
//	bool UseOAuth;
//	bool UseSessionToken;
//	CreatorClient Client;
} ThreadRequestSecurity;

static CreatorList _ThreadRequestSecurityList = NULL;

static CreatorSemaphore _ThreadLock = NULL;

static uint  _NextTaskID = 0;

static void ThreadCallbackWrapper(void *context);


void CreatorThread_ClearLastError(void)
{
	if (_ThreadErrors)
	{
		uint threadID = CreatorThread_GetThreadID(NULL);
		CreatorSemaphore_Wait(_ThreadLock,1);
		uint index;
		for(index = 0; index < CreatorList_GetCount(_ThreadErrors); index++)
		{
			ThreadError *threadError = CreatorList_GetItem(_ThreadErrors, index);
			if (threadError && (threadError->ThreadID == threadID))
			{
				CreatorList_RemoveAt(_ThreadErrors, index);
				Creator_MemFree((void **)&threadError);
				break;
			}
		}
		CreatorSemaphore_Release(_ThreadLock,1);
	}
}

void CreatorThread_Free(CreatorThread *self)
{
	if (self && *self)
	{
		ThreadInfo *threadInfo = (ThreadInfo*)*self;
		xTaskHandle threadID = threadInfo->ThreadID;
		Creator_MemFree((void **)self);
		vTaskDelete(threadID);
	}
}

//CreatorClient CreatorThread_GetClient()
//{
//	CreatorClient result = CreatorClient_GetDefaultClient();
//	CreatorSemaphore_Wait(_ThreadLock,1);
//	if (_ThreadRequestSecurityList)
//	{
//		uint threadID = CreatorThread_GetThreadID(NULL);
//		uint index;
//		for(index = 0; index < CreatorList_GetCount(_ThreadRequestSecurityList); index++)
//		{
//			ThreadRequestSecurity *requestSecurity = CreatorList_GetItem(_ThreadRequestSecurityList, index);
//			if (requestSecurity && (requestSecurity->ThreadID == threadID) && requestSecurity->Client)
//			{
//				result = requestSecurity->Client;
//				break;
//			}
//		}
//	}
//	CreatorSemaphore_Release(_ThreadLock,1);
//	return result;
//}

CreatorErrorType CreatorThread_GetLastError(void)
{
	CreatorErrorType result = CreatorError_NoError;
	if (_ThreadErrors)
	{
		uint threadID = CreatorThread_GetThreadID(NULL);
		CreatorSemaphore_Wait(_ThreadLock,1);
		uint index;
		for(index = 0; index < CreatorList_GetCount(_ThreadErrors); index++)
		{
			ThreadError *threadError = CreatorList_GetItem(_ThreadErrors, index);
			if (threadError && (threadError->ThreadID == threadID))
			{
				result = threadError->Error;
				break;
			}
		}
		CreatorSemaphore_Release(_ThreadLock,1);
	}
	return result;
}

uint CreatorThread_GetThreadID(CreatorThread self)
{
	uint result = 0;
	xTaskHandle currentTask;
	if (self)
	{
		ThreadInfo *threadInfo = (ThreadInfo*)self;
		currentTask = threadInfo->ThreadID;
	}
	else
	{
		currentTask = xTaskGetCurrentTaskHandle();
	}
	result = (uint)uxTaskGetTaskNumber(currentTask);
	return result;
}

//bool CreatorThread_GetUseOAuth(void)
//{
//	bool result = true;
//	CreatorSemaphore_Wait(_ThreadLock,1);
//	if (_ThreadRequestSecurityList)
//	{
//		uint threadID = CreatorThread_GetThreadID(NULL);
//		uint index;
//		for(index = 0; index < CreatorList_GetCount(_ThreadRequestSecurityList); index++)
//		{
//			ThreadRequestSecurity *requestSecurity = CreatorList_GetItem(_ThreadRequestSecurityList, index);
//			if (requestSecurity && (requestSecurity->ThreadID == threadID))
//			{
//				result = requestSecurity->UseOAuth;
//				break;
//			}
//		}
//	}
//	CreatorSemaphore_Release(_ThreadLock,1);
//	return result;
//}

//bool CreatorThread_GetUseSessionToken(void)
//{
//	bool result = true;
//	CreatorSemaphore_Wait(_ThreadLock,1);
//	if (_ThreadRequestSecurityList)
//	{
//		uint threadID = CreatorThread_GetThreadID(NULL);
//		uint index;
//		for(index = 0; index < CreatorList_GetCount(_ThreadRequestSecurityList); index++)
//		{
//			ThreadRequestSecurity *requestSecurity = CreatorList_GetItem(_ThreadRequestSecurityList, index);
//			if (requestSecurity && (requestSecurity->ThreadID == threadID))
//			{
//				result = requestSecurity->UseSessionToken;
//				break;
//			}
//		}
//	}
//	CreatorSemaphore_Release(_ThreadLock,1);
//	return result;
//}

void CreatorThread_Initialise(void)
{
	if (!_ThreadLock)
		_ThreadLock = CreatorSemaphore_New(1,0);
}

void CreatorThread_Join(CreatorThread self)
{
//	ThreadInfo *threadInfo = (ThreadInfo*)self;
//	pthread_join(threadInfo->ThreadID, NULL);
}

//void CreatorThread_LogoutClient(CreatorClient client)
//{
//	CreatorSemaphore_Wait(_ThreadLock,1);
//	if (!_ThreadRequestSecurityList)
//		_ThreadRequestSecurityList = CreatorList_New(10);
//	uint index = 0;
//	while (index < CreatorList_GetCount(_ThreadRequestSecurityList))
//	{
//		ThreadRequestSecurity *requestSecurity = CreatorList_GetItem(_ThreadRequestSecurityList, index);
//		if (requestSecurity && (requestSecurity->Client == client))
//		{
//			requestSecurity->Client = NULL;
//			if (requestSecurity->UseSessionToken && requestSecurity->UseOAuth && !requestSecurity->Client)
//			{
//				CreatorList_RemoveAt(_ThreadRequestSecurityList, index);
//				Creator_MemFree((void **)&requestSecurity);
//			}
//			else
//				index++;
//		}
//		else
//			index++;
//	}
//	CreatorSemaphore_Release(_ThreadLock,1);
//}

CreatorThread CreatorThread_New(const char *name, uint priority, uint stackSize, CreatorThread_Callback runnable, void *context)
{
	ThreadInfo *result = (ThreadInfo*)Creator_MemAlloc(sizeof(ThreadInfo));
	if (result)
	{
		result->Runnable = runnable;
		result->Context = context;
		result->ThreadID = NULL;
		if (stackSize == 0)
			stackSize = CREATOR_STACK_SIZE;
		xTaskCreate(ThreadCallbackWrapper, name, stackSize, result, priority + (tskIDLE_PRIORITY + 1), &result->ThreadID);
		if (!result->ThreadID)
		{
			Creator_MemFree((void **)&result);
		}
		else
		{
			vTaskSetTaskNumber(result->ThreadID, _NextTaskID++);
		}
	}
	return (CreatorThread)result;
}

//void CreatorThread_SetClient(CreatorClient client)
//{
//	uint threadID = CreatorThread_GetThreadID(NULL);
//	CreatorSemaphore_Wait(_ThreadLock,1);
//	if (!_ThreadRequestSecurityList)
//		_ThreadRequestSecurityList = CreatorList_New(10);
//	uint index;
//	bool found = false;
//	for(index = 0; index < CreatorList_GetCount(_ThreadRequestSecurityList); index++)
//	{
//		ThreadRequestSecurity *requestSecurity = CreatorList_GetItem(_ThreadRequestSecurityList, index);
//		if (requestSecurity && (requestSecurity->ThreadID == threadID))
//		{
//			requestSecurity->Client = client;
//			if (requestSecurity->UseSessionToken && requestSecurity->UseOAuth && !requestSecurity->Client)
//			{
//				CreatorList_RemoveAt(_ThreadRequestSecurityList, index);
//				Creator_MemFree((void **)&requestSecurity);
//			}
//			found = true;
//			break;
//		}
//	}
//	if (!found && client)
//	{
//		ThreadRequestSecurity *requestSecurity = Creator_MemAlloc(sizeof(ThreadRequestSecurity));
//		if (requestSecurity)
//		{
//			requestSecurity->ThreadID = threadID;
//			requestSecurity->UseOAuth = true;
//			requestSecurity->UseSessionToken = true;
//			requestSecurity->Client = client;
//			CreatorList_Add(_ThreadRequestSecurityList, requestSecurity);
//		}
//	}
//	CreatorSemaphore_Release(_ThreadLock,1);
//}

//void CreatorThread_SetUseOAuth(bool value)
//{
//	uint threadID = CreatorThread_GetThreadID(NULL);
//	CreatorSemaphore_Wait(_ThreadLock,1);
//	if (!_ThreadRequestSecurityList)
//		_ThreadRequestSecurityList = CreatorList_New(10);
//	uint index;
//	bool found = false;
//	for(index = 0; index < CreatorList_GetCount(_ThreadRequestSecurityList); index++)
//	{
//		ThreadRequestSecurity *requestSecurity = CreatorList_GetItem(_ThreadRequestSecurityList, index);
//		if (requestSecurity && (requestSecurity->ThreadID == threadID))
//		{
//			requestSecurity->UseOAuth = value;
//			if (requestSecurity->UseSessionToken && requestSecurity->UseOAuth && !requestSecurity->Client)
//			{
//				CreatorList_RemoveAt(_ThreadRequestSecurityList, index);
//				Creator_MemFree((void **)&requestSecurity);
//			}
//			found = true;
//			break;
//		}
//	}
//	if (!found && !value)
//	{
//		ThreadRequestSecurity *requestSecurity = Creator_MemAlloc(sizeof(ThreadRequestSecurity));
//		if (requestSecurity)
//		{
//			requestSecurity->ThreadID = threadID;
//			requestSecurity->UseOAuth = value;
//			requestSecurity->UseSessionToken = true;
//			requestSecurity->Client = NULL;
//			CreatorList_Add(_ThreadRequestSecurityList, requestSecurity);
//		}
//	}
//	CreatorSemaphore_Release(_ThreadLock,1);
//}
//
//void CreatorThread_SetUseSessionToken(bool value)
//{
//	uint threadID = CreatorThread_GetThreadID(NULL);
//	CreatorSemaphore_Wait(_ThreadLock,1);
//	if (!_ThreadRequestSecurityList)
//		_ThreadRequestSecurityList = CreatorList_New(10);
//	uint index;
//	bool found = false;
//	for(index = 0; index < CreatorList_GetCount(_ThreadRequestSecurityList); index++)
//	{
//		ThreadRequestSecurity *requestSecurity = CreatorList_GetItem(_ThreadRequestSecurityList, index);
//		if (requestSecurity && (requestSecurity->ThreadID == threadID))
//		{
//			requestSecurity->UseSessionToken = value;
//			if (requestSecurity->UseSessionToken && requestSecurity->UseOAuth && !requestSecurity->Client)
//			{
//				CreatorList_RemoveAt(_ThreadRequestSecurityList, index);
//				Creator_MemFree((void **)&requestSecurity);
//			}
//			found = true;
//			break;
//		}
//	}
//	if (!found && !value)
//	{
//		ThreadRequestSecurity *requestSecurity = Creator_MemAlloc(sizeof(ThreadRequestSecurity));
//		if (requestSecurity)
//		{
//			requestSecurity->ThreadID = threadID;
//			requestSecurity->UseOAuth = true;
//			requestSecurity->UseSessionToken = value;
//			requestSecurity->Client = NULL;
//			CreatorList_Add(_ThreadRequestSecurityList, requestSecurity);
//		}
//	}
//	CreatorSemaphore_Release(_ThreadLock,1);
//}

void CreatorThread_SetError(CreatorErrorType error)
{
	if (error == CreatorError_NoError)
		CreatorThread_ClearLastError();
	else
	{
		uint threadID = CreatorThread_GetThreadID(NULL);
		if (!_ThreadLock)
			_ThreadLock = CreatorSemaphore_New(1,0);
		CreatorSemaphore_Wait(_ThreadLock,1);
		if (!_ThreadErrors)
			_ThreadErrors = CreatorList_New(10);
		uint index;
		bool found = false;
		for(index = 0; index < CreatorList_GetCount(_ThreadErrors); index++)
		{
			ThreadError *threadError = CreatorList_GetItem(_ThreadErrors, index);
			if (threadError && (threadError->ThreadID == threadID))
			{
				threadError->Error = error;
				found = true;
				break;
			}
		}
		if (!found)
		{
			ThreadError *threadError = Creator_MemAlloc(sizeof(ThreadError));
			if (threadError)
			{
				threadError->ThreadID = threadID;
				threadError->Error = error;
				CreatorList_Add(_ThreadErrors, threadError);
			}
		}
		CreatorSemaphore_Release(_ThreadLock,1);
	}
}

void CreatorThread_Shutdown(void)
{
	if (_ThreadErrors)
		 CreatorList_Free(&_ThreadErrors, true);
	if (_ThreadRequestSecurityList)
		 CreatorList_Free(&_ThreadRequestSecurityList, true);
	if (_ThreadLock)
		CreatorSemaphore_Free(&_ThreadLock);
}

bool CreatorThread_Sleep(CreatorThread self, uint seconds)
{
	bool result = false;
	if (seconds > 0) 
	{
		if (self)
		{
			ThreadInfo *threadInfo = (ThreadInfo*)self;
			xTaskHandle currentTask =  xTaskGetCurrentTaskHandle();
			result = (currentTask == threadInfo->ThreadID);
		}
		else
			result = true;
		if (result)
		{
			portTickType ticks = (seconds * CreatorTimer_GetTicksPerSecond());
			vTaskDelay(ticks);
		}
//		struct timespec sleepDuration;
//		sleepDuration.tv_nsec = 0;
//		sleepDuration.tv_sec = time(NULL) + seconds;
//		pthread_mutex_lock(&threadInfo->sleepMutex);
//		int waitResult = pthread_cond_timedwait(&threadInfo->sleepCondition, &threadInfo->sleepMutex, &sleepDuration);
//		pthread_mutex_unlock(&threadInfo->sleepMutex);
//		return (waitResult != ETIMEDOUT);
	}
	return result;
}

bool CreatorThread_SleepMilliseconds(CreatorThread self, uint milliseconds)
{
	bool result = false;
	if (milliseconds > 0)
	{
		if (self)
		{
			ThreadInfo *threadInfo = (ThreadInfo*)self;
			xTaskHandle currentTask =  xTaskGetCurrentTaskHandle();
			result = (currentTask == threadInfo->ThreadID);
		}
		else
			result = true;
		if (result)
		{
			portTickType ticks = (milliseconds * CreatorTimer_GetTicksPerSecond())/1000;
			if (ticks == 0)
				ticks = 1;
			vTaskDelay(ticks);
		}
	}
	return result;
}

bool CreatorThread_SleepTicks(CreatorThread self, uint ticks)
{
	bool result = false;
	if (ticks > 0)
	{
		if (self)
		{
			ThreadInfo *threadInfo = (ThreadInfo*)self;
			xTaskHandle currentTask =  xTaskGetCurrentTaskHandle();
			result = (currentTask == threadInfo->ThreadID);
		}
		else
			result = true;
		if (result)
			vTaskDelay(ticks);
	}
	return result;
}

void CreatorThread_Wakeup(CreatorThread self)
{
//	ThreadInfo *threadInfo = (ThreadInfo*)self;
//	pthread_mutex_lock(&threadInfo->sleepMutex);
//	pthread_cond_signal(&threadInfo->sleepCondition);
//	pthread_mutex_unlock(&threadInfo->sleepMutex);
}



static void ThreadCallbackWrapper(void *context)
{
	ThreadInfo *threadInfo = (ThreadInfo*)context;
	if (threadInfo)
		threadInfo->Runnable(threadInfo, threadInfo->Context);
	vTaskDelete(NULL);
}

#endif
