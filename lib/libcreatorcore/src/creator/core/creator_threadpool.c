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

/*! \file creator_threadpool.c
 *  \brief LibCreatorCore .
 */

#include "creator/core/base_types.h"
#include "creator/core/creator_queue.h"
#include "creator/core/creator_list.h"
#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_threading.h"
#include "creator/core/creator_threadpool.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

typedef struct
{
    CreatorThreadPool_Callback Runnable;
    void *RunnableContext;
} ThreadPoolTask;


typedef struct
{
    CreatorQueue Tasks;
    CreatorList Threads;
    CreatorSemaphore Lock;
    uint MinThreads;
    uint MaxThreads;
    uint Priority;
    uint StackSize;
    bool Terminate;
} ThreadPool;

static void executeTask(CreatorThread thread, void *context);

void CreatorThreadPool_Free(CreatorThreadPool *self)
{
    if (self && *self)
    {
        ThreadPool *threadPool = *self;
        int count = 0;
        threadPool->Terminate = true;
        if (threadPool->Lock)
        {
            CreatorSemaphore_Wait(threadPool->Lock, 1);
            if (threadPool->Threads)
                count = CreatorList_GetCount(threadPool->Threads);
            CreatorSemaphore_Release(threadPool->Lock, 1);
        }
        if (threadPool->Tasks)
        {
            ThreadPoolTask *task = CreatorQueue_DequeueWaitFor(threadPool->Tasks, 1);
            while (task != NULL)
            {
                Creator_MemFree((void **)&task);
                task = CreatorQueue_DequeueWaitFor(threadPool->Tasks, 1);
            }
            CreatorQueue_Free(&threadPool->Tasks);
        }
        if (threadPool->Threads && (count == 0))
        {
            CreatorList_Free(&threadPool->Threads, false);
        }
        if (threadPool->Lock && (count == 0))
            CreatorSemaphore_Free(&threadPool->Lock);
        if (count == 0)
            Creator_MemFree(self);
        else
            self = NULL;
    }
}

CreatorThreadPool CreatorThreadPool_New(uint minThreads, uint maxThreads, uint priority, uint stackSize)
{
    CreatorThreadPool result = NULL;
    ThreadPool *threadPool = (ThreadPool*)Creator_MemAlloc(sizeof(ThreadPool));
    if (threadPool)
    {
        memset(threadPool, 0, sizeof(ThreadPool));
        bool failed = false;
        threadPool->Lock = CreatorSemaphore_New(1, 0);
        if (threadPool->Lock)
        {
            threadPool->Tasks = CreatorQueue_NewBlocking(10);
            if (threadPool->Tasks)
            {
                threadPool->Threads = CreatorList_New(maxThreads);
                if (threadPool->Threads)
                {
                    if (minThreads < 1)
                        minThreads = 1;
                    if (maxThreads < minThreads)
                        maxThreads = minThreads;
                    threadPool->MinThreads = minThreads;
                    threadPool->MaxThreads = maxThreads;
                    threadPool->Priority = priority;
                    threadPool->StackSize = stackSize;
                    result = threadPool;
                }
                else
                    failed = true;
            }
            else
                failed = true;
        }
        else
            failed = true;
        if (failed)
        {
            CreatorThreadPool_Free((CreatorThreadPool) & threadPool);
        }
    }
    return result;
}

bool CreatorThreadPool_AddTask(CreatorThreadPool self, CreatorThreadPool_Callback runnable, void *context)
{
    bool result = false;
    ThreadPool *threadPool = self;
    if (threadPool && runnable && !threadPool->Terminate)
    {
        ThreadPoolTask *task = (ThreadPoolTask*)Creator_MemAlloc(sizeof(ThreadPoolTask));
        if (task)
        {
            task->Runnable = runnable;
            task->RunnableContext = context;
            if (CreatorQueue_Enqueue(threadPool->Tasks, task))
            {
                result = true;
                int count = CreatorList_GetCount(threadPool->Threads);
                if (count < threadPool->MinThreads)
                {
                    CreatorSemaphore_Wait(threadPool->Lock, 1);
                    count = CreatorList_GetCount(threadPool->Threads);
                    if (count < threadPool->MinThreads)
                    {
                        CreatorThread thread = CreatorThread_New("ThreadPool", threadPool->Priority, threadPool->StackSize, executeTask, threadPool);
                        if (thread)
                            CreatorList_Add(threadPool->Threads, thread);
                    }
                    CreatorSemaphore_Release(threadPool->Lock, 1);
                }
            }
        }
    }
    return result;
}

void executeTask(CreatorThread thread, void *context)
{
    ThreadPool *threadPool = context;
    if (threadPool)
    {
        while (!threadPool->Terminate)
        {
            ThreadPoolTask *task = CreatorQueue_DequeueWaitFor(threadPool->Tasks, 2000);
            if (task)
            {
                CreatorThread_ClearLastError();
                task->Runnable(task->RunnableContext);
                Creator_MemFree((void **)&task);
            }
        }
        CreatorSemaphore_Wait(threadPool->Lock, 1);
        CreatorList_Remove(threadPool->Threads, thread);
        int count = CreatorList_GetCount(threadPool->Threads);
        CreatorSemaphore_Release(threadPool->Lock, 1);
        if (count == 0)
        {
            CreatorList_Free(&threadPool->Threads, false);
            CreatorSemaphore_Free(&threadPool->Lock);
            Creator_MemFree((void **)&threadPool);
        }
    }
    CreatorThread_Free(&thread);
}
