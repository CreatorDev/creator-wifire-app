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
#include <time.h>
#include <string.h>

#define CREATOR_DEBUG_ON
#include "creator/core/creator_debug.h"

#include "creator/core/creator_threading.h"
#include "creator/core/creator_memalloc.h"

#include "creator/creator.h"

#include "creator_task_scheduler.h"

//declare it that way because it's easier while we don't have a proper build system
extern CreatorEventLog CreatorEventLogs_newItem(CreatorEventLogs logs);

typedef struct LogItemImpl LogItem;

struct LogItemImpl
{
    CreatorEventSeverity severity;
    char *category;
    CreatorDatetime moment;
    const char *message;
    size_t msgLength;

    //linked list next item
    LogItem *pNextItem;
};

static const char szLogCategory[] = "libcreatorapi.creator_log";

static LogItem *LogItem_New(CreatorLogLevel level, char *szMsg);
static void LogItem_Destroy(LogItem *pItem);

static CreatorSemaphore logsSemaphore;
static LogItem *pFirstItem;
static LogItem *pLastItem;

static void appendLogItem(LogItem *item);

//process of posting log messages
#define LOG_SIZE_LIMIT 32000
static CreatorTaskID logPostingTaskID;
static void logsPoster(void*);

bool CreatorLog_Initialise()
{
    pFirstItem = NULL;
    pLastItem = NULL;
    logsSemaphore = CreatorSemaphore_New(1, 0);
    return logsSemaphore != NULL;
}

void CreatorLog_Shutdown()
{
    CreatorSemaphore_Free(&logsSemaphore);
}

void CreatorLog_SetLevel(CreatorLogLevel level)
{
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
    char fullMsg[2048];
    vsnprintf(fullMsg, sizeof(fullMsg), message, vl);

    LogItem *pNewItem = LogItem_New(level, fullMsg);
    appendLogItem(pNewItem);
}

static LogItem *LogItem_New(CreatorLogLevel level, const char *szMsg)
{
    LogItem *pNewItem = Creator_MemAlloc(sizeof(LogItem));
    if (pNewItem)
    {
        pNewItem->pNextItem = NULL;
        pNewItem->severity = CREATOR_EVENTSEVERITY_ERROR;
        pNewItem->category = szLogCategory;
        pNewItem->moment = time(NULL);

        switch (level) {
            case CreatorLogLevel_Debug:
                pNewItem->severity = CREATOR_EVENTSEVERITY_DEBUG;
                break;
            case CreatorLogLevel_Info:
                pNewItem->severity = CREATOR_EVENTSEVERITY_INFORMATION;
                break;
            case CreatorLogLevel_Warning:
                pNewItem->severity = CREATOR_EVENTSEVERITY_WARNING;
                break;
            case CreatorLogLevel_Error:
                pNewItem->severity = CREATOR_EVENTSEVERITY_ERROR;
                break;
        }

        if (szMsg)
        {
            size_t msgSize = strlen(szMsg) + 1;
            pNewItem->message = Creator_MemAlloc(msgSize);
            if (pNewItem->message)
            {
                strcpy(pNewItem->message, szMsg);
                pNewItem->msgLength = msgSize;
            }
            else
            {
                Creator_MemFree((void **)&pNewItem);
            }
        }
    }
    if (!pNewItem)
    {
        fprintf(stderr, "Ran out of memory for saving log items\n");
    }
    return pNewItem;
}

static void LogItem_Destroy(LogItem *pItem)
{
    if (pItem)
    {
        if (pItem->message)
        {
            Creator_MemFree(&pItem->message);
        }
        Creator_MemFree(&pItem);
    }
}

static void appendLogItem(LogItem *item)
{
    CreatorSemaphore_Wait(logsSemaphore, 1);
    if (pLastItem == NULL)
    {
        pFirstItem = item;
        pLastItem = item;
    }
    else
    {
        pLastItem->pNextItem = item;
        pLastItem = item;
    }
    if (!logPostingTaskID)
    {
        logPostingTaskID = CreatorScheduler_ScheduleTask(logsPoster, NULL, 5, false);
    }
    CreatorSemaphore_Release(logsSemaphore, 1);
}

static void logsPoster(void *pArg)
{
    CreatorMemoryManager memoryManager = CreatorMemoryManager_New();
    LogItem *pFirstSentItem = NULL, *pLastUnsentItem = NULL;

    CreatorSemaphore_Wait(logsSemaphore, 1);
    CreatorEventLogs logs = CreatorEventLogsNew(memoryManager);
    fprintf(stdout, "Posting batch of events to webservice\n");

    pFirstSentItem = pFirstItem;
    pLastUnsentItem = NULL;

    LogItem *pCurrentItem = pFirstItem, *pNextItem;
    size_t cumulatedSize = 0;
    while (pCurrentItem)
    {
        CreatorEventLog log = CreatorEventLogs_newItem(logs);

        CreatorEventLog_SetCategory(log, pCurrentItem->category);
        CreatorEventLog_SetEventDate(log, pCurrentItem->moment);
        CreatorEventLog_SetMessage(log, pCurrentItem->message);
        CreatorEventLog_SetSeverity(log, pCurrentItem->severity);
        cumulatedSize += pCurrentItem->msgLength;

        pNextItem = pCurrentItem->pNextItem;
        pCurrentItem = pLastUnsentItem = pNextItem;

        if (cumulatedSize > LOG_SIZE_LIMIT)
        {
            fprintf(stdout, "Reached message limit, sending this first batch\n");
            break;
        }
    }
    pFirstItem = pCurrentItem;
    if (!pCurrentItem)
    {
        pLastItem = NULL;
        logPostingTaskID = 0;
    }
    else
    {
        logPostingTaskID = CreatorScheduler_ScheduleTask(logsPoster, NULL, 0, false);
        fprintf(stdout, "Scheduled new batch of logs to be sent\n");
    }
    CreatorSemaphore_Release(logsSemaphore, 1);

    //do this outside of the mutex to avoid re-entrance
    CreatorAPI api = CreatorJob_GetAPI(memoryManager);
    CreatorAPI_eventlogsEventLogs(api, logs);
    if (CreatorJob_IsValid(memoryManager))
    {
        //clearing sent log items
        CreatorSemaphore_Wait(logsSemaphore, 1);
        pCurrentItem = pFirstSentItem;
        while (pCurrentItem != pLastUnsentItem)
        {
            pNextItem = pCurrentItem->pNextItem;
            LogItem_Destroy(pCurrentItem);
            pCurrentItem = pNextItem;
        }
        if (pFirstItem == pFirstSentItem)
        {
            pFirstItem = pCurrentItem;
            if (!pCurrentItem)
            {
                pLastItem = NULL;
            }
        }
        CreatorSemaphore_Release(logsSemaphore, 1);
    }
    CreatorMemoryManager_Free(&memoryManager);
}
