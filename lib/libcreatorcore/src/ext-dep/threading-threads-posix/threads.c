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

//enable nanosleep
#ifndef _POSIX_C_SOURCE
	#define _POSIX_C_SOURCE 199506L
#endif
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>

#include "creator/core/creator_time.h"
#include "creator/core/creator_threading.h"
#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_list.h"
#include "creator/core/client.h"

typedef struct
{
    CreatorThread_Callback Runnable;
    void *RunnableContext;
    pthread_t ThreadID;
    pthread_cond_t SleepCondition;
    pthread_mutex_t SleepMutex;
    CreatorSemaphore Lock;
    bool Exited;
}ThreadInfo;

static CreatorList _Threads = NULL;

typedef struct
{
    pthread_t ThreadID;
    CreatorErrorType Error;
}ThreadError;

static CreatorList _ThreadErrors = NULL;

typedef struct
{
    pthread_t ThreadID;
//	bool UseOAuth;
//	bool UseSessionToken;
//	CreatorClient Client;
}ThreadRequestSecurity;

static CreatorList _ThreadRequestSecurityList = NULL;

static CreatorSemaphore _ThreadLock = NULL;

static pthread_t GetThreadID(CreatorThread self);
static ThreadInfo *GetThreadInfo(CreatorThread self);
static void *ThreadCallbackWrapper(void *context);

void CreatorThread_ClearLastError(void)
{
    if (_ThreadErrors)
    {
        pthread_t threadID = GetThreadID(NULL);
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
        pthread_t threadID = GetThreadID(NULL);
        ThreadInfo *threadInfo = (ThreadInfo*)*self;
        if (_ThreadLock)
            CreatorSemaphore_Wait(_ThreadLock,1);
        if (_Threads)
            CreatorList_Remove(_Threads, threadInfo);
        if (_ThreadLock)
            CreatorSemaphore_Release(_ThreadLock,1);
        if (threadInfo->Lock)
        {
            CreatorSemaphore_Wait(threadInfo->Lock,1);
        }
        if (!threadInfo->Exited)
        {
            if (threadInfo->ThreadID == threadID)
            {
                pthread_detach(threadInfo->ThreadID);
            }
            else
            {
                pthread_cancel(threadInfo->ThreadID);
                pthread_join(threadInfo->ThreadID, NULL);
            }
            threadInfo->Exited = true;
        }
        if (threadInfo->Lock)
        {
            CreatorSemaphore_Release(threadInfo->Lock,1);
        }
        pthread_cond_destroy(&threadInfo->SleepCondition);
        pthread_mutex_destroy(&threadInfo->SleepMutex);
        if (threadInfo->Lock)
        {
            CreatorSemaphore_Free(&threadInfo->Lock);
        }
        Creator_MemFree((void **)self);
    }
}

//CreatorClient CreatorThread_GetClient()
//{
//	CreatorClient result = CreatorClient_GetDefaultClient();
//	CreatorSemaphore_Wait(_ThreadLock,1);
//	if (_ThreadRequestSecurityList)
//	{
//		pthread_t threadID = GetThreadID(NULL);
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
        pthread_t threadID = GetThreadID(NULL);
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
    return (uint)GetThreadID(self);
}

//bool CreatorThread_GetUseOAuth(void)
//{
//	bool result = true;
//	CreatorSemaphore_Wait(_ThreadLock,1);
//	if (_ThreadRequestSecurityList)
//	{
//		pthread_t threadID = GetThreadID(NULL);
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
//
//bool CreatorThread_GetUseSessionToken(void)
//{
//	bool result = true;
//	CreatorSemaphore_Wait(_ThreadLock,1);
//	if (_ThreadRequestSecurityList)
//	{
//		pthread_t threadID = GetThreadID(NULL);
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
    if (!_Threads)
        _Threads = CreatorList_New(5);
    if (!_ThreadLock)
        _ThreadLock = CreatorSemaphore_New(1,0);
}

void CreatorThread_Join(CreatorThread self)
{
    ThreadInfo *threadInfo = (ThreadInfo*)self;
    if (threadInfo)
    {
        if (!threadInfo->Exited)
        {
            if (threadInfo->Lock)
            {
                CreatorSemaphore_Wait(threadInfo->Lock,1);
            }
            if (!threadInfo->Exited)
            {
                threadInfo->Exited = true;
                pthread_join(threadInfo->ThreadID, NULL);
            }
            if (threadInfo->Lock)
            {
                CreatorSemaphore_Release(threadInfo->Lock,1);
            }
        }
    }
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
    if (!_Threads)
        _Threads = CreatorList_New(5);
    (void)name;
    (void)priority;
    (void)stackSize;
    ThreadInfo *threadInfo = (ThreadInfo*)Creator_MemAlloc(sizeof(ThreadInfo));
    if (!threadInfo) {
        return NULL;
    }
    threadInfo->Runnable = runnable;
    threadInfo->RunnableContext = context;
    threadInfo->Lock = CreatorSemaphore_New(1,0);
    threadInfo->Exited = false;
    pthread_cond_init(&threadInfo->SleepCondition, NULL);
    pthread_mutex_init(&threadInfo->SleepMutex, NULL);

    if (pthread_create(&threadInfo->ThreadID, NULL, ThreadCallbackWrapper, threadInfo) != 0)
    {
        if (threadInfo->Lock)
        {
            CreatorSemaphore_Free(&threadInfo->Lock);
        }
        Creator_MemFree((void **)&threadInfo);
        return NULL;
    }
    CreatorSemaphore_Wait(_ThreadLock,1);
    CreatorList_Add(_Threads, threadInfo);
    CreatorSemaphore_Release(_ThreadLock,1);
    return (CreatorThread)threadInfo;
}

void CreatorThread_SetError(CreatorErrorType error)
{
    if (error == CreatorError_NoError)
        CreatorThread_ClearLastError();
    else
    {
        pthread_t threadID = GetThreadID(NULL);
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

//void CreatorThread_SetClient(CreatorClient client)
//{
//	pthread_t threadID = GetThreadID(NULL);
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
//
//void CreatorThread_SetUseOAuth(bool value)
//{
//	pthread_t threadID = GetThreadID(NULL);
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
//	pthread_t threadID = GetThreadID(NULL);
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

bool CreatorThread_Sleep(CreatorThread self, uint seconds)
{
    bool result = false;
    if (seconds > 0)
    {
        ThreadInfo *threadInfo = GetThreadInfo(self);
        struct timespec sleepDuration;
        sleepDuration.tv_nsec = 0;
        if (threadInfo)
        {
            struct timeval currentTime;
            gettimeofday(&currentTime, NULL);
            sleepDuration.tv_sec = currentTime.tv_sec + seconds;
            sleepDuration.tv_nsec += (currentTime.tv_usec * 1000);
            pthread_mutex_lock(&threadInfo->SleepMutex);
            int waitResult = pthread_cond_timedwait(&threadInfo->SleepCondition, &threadInfo->SleepMutex, &sleepDuration);
            pthread_mutex_unlock(&threadInfo->SleepMutex);
            result = (waitResult != ETIMEDOUT);
        }
        else
        {

            sleepDuration.tv_sec = seconds;
            sleepDuration.tv_nsec = 0;
            struct timespec remaining;
            errno = 0;
            while (nanosleep(&sleepDuration, &remaining) == -1)
            {
                if (errno == EINTR)
                {
                    sleepDuration.tv_sec = remaining.tv_sec;
                    sleepDuration.tv_nsec = remaining.tv_nsec;
                }
                else
                    break;
            }

//			int remaining = sleep(seconds);
//			while (remaining > 0)
//			{
//				seconds = remaining;
//				remaining = sleep(seconds);
//			}
        }
    }
    return result;
}

bool CreatorThread_SleepMilliseconds(CreatorThread self, uint milliseconds)
{
    bool result = false;
    if (milliseconds > 0)
    {
        ThreadInfo *threadInfo = GetThreadInfo(self);
        struct timespec sleepDuration;
        sleepDuration.tv_nsec =  milliseconds * 1000000;
        if (threadInfo)
        {
            struct timeval currentTime;
            gettimeofday(&currentTime, NULL);
            sleepDuration.tv_sec = currentTime.tv_sec;
            sleepDuration.tv_nsec += (currentTime.tv_usec * 1000);
            pthread_mutex_lock(&threadInfo->SleepMutex);
            int waitResult = pthread_cond_timedwait(&threadInfo->SleepCondition, &threadInfo->SleepMutex, &sleepDuration);
            pthread_mutex_unlock(&threadInfo->SleepMutex);
            result = (waitResult != ETIMEDOUT);
        }
        else
        {
            sleepDuration.tv_sec = 0;
            struct timespec remaining;
            errno = 0;
            while (nanosleep(&sleepDuration, &remaining) == -1)
            {
                if (errno == EINTR)
                    sleepDuration.tv_nsec = remaining.tv_nsec;
                else
                    break;
            }
        }
    }
    return result;
}

bool CreatorThread_SleepTicks(CreatorThread self, uint ticks)
{
    bool result = false;
    if (ticks > 0)
    {
        ThreadInfo *threadInfo = GetThreadInfo(self);
        struct timespec sleepDuration;
        sleepDuration.tv_nsec = ticks;
        if (threadInfo)
        {
            struct timeval currentTime;
            gettimeofday(&currentTime, NULL);
            sleepDuration.tv_sec = currentTime.tv_sec;
            sleepDuration.tv_nsec += (currentTime.tv_usec * 1000);
            pthread_mutex_lock(&threadInfo->SleepMutex);
            int waitResult = pthread_cond_timedwait(&threadInfo->SleepCondition, &threadInfo->SleepMutex, &sleepDuration);
            pthread_mutex_unlock(&threadInfo->SleepMutex);
            result = (waitResult != ETIMEDOUT);
        }
        else
        {
            sleepDuration.tv_sec = 0;
            struct timespec remaining;
            while (nanosleep(&sleepDuration, &remaining) == -1)
            {
                if (errno == EINTR)
                    sleepDuration.tv_nsec = remaining.tv_nsec;
                else
                    break;
            }
        }
    }
    return result;
}


void CreatorThread_Shutdown(void)
{
    if (_ThreadErrors)
         CreatorList_Free(&_ThreadErrors, true);
    if (_ThreadRequestSecurityList)
         CreatorList_Free(&_ThreadRequestSecurityList, true);
    if (_Threads)
         CreatorList_Free(&_Threads, false);
    if (_ThreadLock)
         CreatorSemaphore_Free(&_ThreadLock);
}

void CreatorThread_Wakeup(CreatorThread self)
{
    ThreadInfo *threadInfo = (ThreadInfo*)self;
    pthread_mutex_lock(&threadInfo->SleepMutex);
    pthread_cond_signal(&threadInfo->SleepCondition);
    pthread_mutex_unlock(&threadInfo->SleepMutex);
}

pthread_t GetThreadID(CreatorThread self)
{
    pthread_t result;
    if (self)
    {
        ThreadInfo *threadInfo = (ThreadInfo*)self;
        result = threadInfo->ThreadID;
    }
    else
    {
        result = pthread_self();
    }
    return result;
}


static ThreadInfo *GetThreadInfo(CreatorThread self)
{
    ThreadInfo *result = NULL;
    if (self)
    {
        result = (ThreadInfo*)self;
    }
    else
    {
        if (_Threads)
        {
            pthread_t threadID = GetThreadID(NULL);
            uint index;
            CreatorSemaphore_Wait(_ThreadLock,1);
            for(index = 0; index < CreatorList_GetCount(_Threads); index++)
            {
                ThreadInfo *threadInfo = CreatorList_GetItem(_Threads, index);
                if (threadInfo && (threadInfo->ThreadID == threadID))
                {
                    result = threadInfo;
                    break;
                }
            }
            CreatorSemaphore_Release(_ThreadLock,1);
        }
    }
    return result;
}

static void *ThreadCallbackWrapper(void *context)
{
    ThreadInfo *threadInfo = (ThreadInfo*)context;
    threadInfo->Runnable(threadInfo, threadInfo->RunnableContext);
    if (threadInfo)
    {
        if (!threadInfo->Exited)
        {
            if (threadInfo->Lock)
            {
                CreatorSemaphore_Wait(threadInfo->Lock,1);
            }
            if (!threadInfo->Exited)
            {
                threadInfo->Exited = true;
                pthread_detach(threadInfo->ThreadID);
            }
            if (threadInfo->Lock)
            {
                CreatorSemaphore_Release(threadInfo->Lock,1);
            }
        }
    }
    return NULL;
}

#endif
