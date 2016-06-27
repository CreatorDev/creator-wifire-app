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

#include <stdbool.h>
#include <time.h>
#include <limits.h>
#include <string.h>

#include "creator/core/creator_task_scheduler.h"

#include "creator/core/creator_threading.h"
#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_debug.h"
//#include "creator/core/servertime.h"

typedef struct
{
    CreatorTaskID TaskID;
    CreatorScheduler_TaskCallback TaskCallback;
    void *TaskContext;
    ulong RecurrenceInterval;
    time_t NextExecutionTime;
} CreatorSchedulerTask;

static uint _TasksCount = 0;
static uint _TasksListSize = 0;
static CreatorSchedulerTask *_TasksList = NULL;
static CreatorThread _TaskRunnerThread = NULL;
static CreatorSemaphore _TaskListLock = NULL;
static CreatorSemaphore _TaskWait = NULL;
//flag indicating when to stop to the task executor thread
static bool _TerminateTaskThread = false;

static CreatorTaskID _LastGeneratedID = 1;

static void TaskRunnerMethod(CreatorThread thread, void *context);
/*
 * must be mutex-protected before entering
 */
static void ReorderTaskList();

bool CreatorScheduler_Initialise(void)
{
    bool result = false;
    //clear up existing variables
    CreatorScheduler_Shutdown();
    _TaskListLock = CreatorSemaphore_New(1, 0);
    if (_TaskListLock)
    {
        _TaskWait = CreatorSemaphore_New(1, 1);
        //create empty list of tasks
        _TasksListSize = 20;
        size_t taskListMemSize = sizeof(CreatorSchedulerTask[_TasksListSize]);
        _TasksList = (CreatorSchedulerTask*)Creator_MemAlloc(taskListMemSize);
        if (_TasksList)
        {
            memset(_TasksList, 0, taskListMemSize);
            _TerminateTaskThread = false;
            //create thread
            CreatorSemaphore_Wait(_TaskListLock, 1);
            _TaskRunnerThread = CreatorThread_New("CreatorScheduler", 0, 0, TaskRunnerMethod, NULL);
            CreatorSemaphore_Release(_TaskListLock, 1);
            if (_TaskRunnerThread)
            {
                result = true;
            }
        }
    }
    if (!result)
    {
        CreatorScheduler_Shutdown();
    }
    return result;
}

CreatorTaskID CreatorScheduler_ScheduleTask(CreatorScheduler_TaskCallback executor, void *context, ulong delayBeforeExecution, bool continuous)
{
    CreatorTaskID result = 0;
    if (executor)
    {
        if (_TaskListLock)
        {
            CreatorSemaphore_Wait(_TaskListLock, 1);
            if (_TasksList && _TaskRunnerThread)
            {
                if (_TasksCount == _TasksListSize)
                {
                    //resize task list
                    size_t newSize = sizeof(CreatorSchedulerTask[_TasksListSize * 2]);
                    size_t growth = newSize - sizeof(CreatorSchedulerTask[_TasksListSize]);
                    CreatorSchedulerTask *newTaskList = Creator_MemRealloc(_TasksList, newSize);
                    if (newTaskList)
                    {
                        memset(&newTaskList[_TasksListSize], 0, growth);
                        _TasksList = newTaskList;
                        _TasksListSize *= 2;
                    }
                }
                if (_TasksCount < _TasksListSize)
                {
                    result = _LastGeneratedID++;
                    _TasksList[_TasksCount].TaskID = result;
                    _TasksList[_TasksCount].TaskCallback = executor;
                    _TasksList[_TasksCount].TaskContext = context;
                    _TasksList[_TasksCount].RecurrenceInterval = (continuous) ? delayBeforeExecution : 0;
                    _TasksList[_TasksCount].NextExecutionTime = Creator_GetTime(NULL) + delayBeforeExecution;
                    _TasksCount++;
                    ReorderTaskList();
                }
            }
            CreatorSemaphore_Release(_TaskListLock, 1);
        }
    }
    else
    {
        Creator_Log(CreatorLogLevel_Error, "NULL executor passed to CreatorScheduler_ScheduleTask");
    }
    if (result)
    {
        CreatorSemaphore_Release(_TaskWait, 1);
        //CreatorThread_Wakeup(_TaskRunnerThread);
        Creator_Log(CreatorLogLevel_Debug, "Scheduling task %u: [%p] in [%lu] seconds", result, executor, delayBeforeExecution);
    }
    else
    {
        Creator_Log(CreatorLogLevel_Error, "Failed to schedule task [%p]", executor);
    }
    return result;
}

void CreatorScheduler_SetTaskInterval(CreatorTaskID taskID, ulong delayBeforeExecution)
{
    if (_TaskListLock)
    {
        CreatorSemaphore_Wait(_TaskListLock, 1);
        if (_TasksList)
        {
            uint i;
            for (i = 0; i < _TasksCount; i++)
            {
                if (_TasksList[i].TaskID == taskID)
                {
                    time_t now = Creator_GetTime(NULL);
                    _TasksList[i].RecurrenceInterval = delayBeforeExecution;
                    _TasksList[i].NextExecutionTime = now + delayBeforeExecution;
                    break;
                }
            }
            ReorderTaskList();
        }
        CreatorSemaphore_Release(_TaskListLock, 1);
    }
}

void CreatorScheduler_UnscheduleTask(CreatorTaskID taskID)
{
    if (_TaskListLock)
    {
        CreatorSemaphore_Wait(_TaskListLock, 1);
        if (_TasksList)
        {
            uint i;
            for (i = 0; i < _TasksCount; i++)
            {
                if (_TasksList[i].TaskID == taskID)
                {
                    _TasksList[i].TaskCallback = NULL;
                    break;
                }
            }
            ReorderTaskList();
        }
        CreatorSemaphore_Release(_TaskListLock, 1);
    }
}

void CreatorScheduler_Shutdown(void)
{
    //stop thread
    if (_TaskRunnerThread)
    {
        _TerminateTaskThread = true;
        CreatorSemaphore_Release(_TaskWait, 1);
        CreatorThread_Wakeup(_TaskRunnerThread);
        CreatorThread_Join(_TaskRunnerThread);
        CreatorThread_Free(&_TaskRunnerThread);
    }

    if (_TaskListLock)
    {
        CreatorSemaphore_Wait(_TaskListLock, 1);
        if (_TasksList)
        {
            Creator_MemFree((void **)&_TasksList);
        }
        _TasksListSize = 0;
        _TasksCount = 0;
        CreatorSemaphore_Release(_TaskListLock, 1);
        CreatorSemaphore_Free(&_TaskListLock);
    }
    else
    {
        if (_TasksList)
        {
            Creator_MemFree((void **)&_TasksList);
        }
        _TasksListSize = 0;
        _TasksCount = 0;
    }
    CreatorSemaphore_Free(&_TaskWait);
}

static void TaskRunnerMethod(CreatorThread thread, void *context)
{
    (void)thread;
    (void)context;
    while (!_TerminateTaskThread)
    {
        if (_TerminateTaskThread)
        {
            break;
        }

        //by default, make it sleep for a *long* while
        //ulong nextTaskIn = 3600*24; // hour
        //ulong nextTaskIn = 60; // minute
        ulong nextTaskIn = 10;
        //this will hold the that are due for execution
        uint taskToExecuteCount = 0;
        //find tasks to execute from sorted list
        CreatorSemaphore_Wait(_TaskListLock, 1);
        CreatorSchedulerTask pTasksToExecute[_TasksCount];
        if (_TasksCount > 0)
        {
            uint i;
            time_t now = Creator_GetTime(NULL);
            for (i = 0; i < _TasksCount; i++)
            {
                if (!_TasksList[i].TaskCallback)
                {
                    //no more executors in the list
                    break;
                }
                if (now >= _TasksList[i].NextExecutionTime)
                {
                    //should execute that on this run
                    pTasksToExecute[taskToExecuteCount++] = _TasksList[i];
                    if (_TasksList[i].RecurrenceInterval)
                    {
                        _TasksList[i].NextExecutionTime = now + _TasksList[i].RecurrenceInterval;
                        if (nextTaskIn > _TasksList[i].RecurrenceInterval)
                        {
                            nextTaskIn = _TasksList[i].RecurrenceInterval;
                        }
                    }
                    else
                    {
                        //this ensures that the task will get deleted during reordering
                        _TasksList[i].TaskCallback = NULL;
                    }
                }
                else
                {
                    //next task is not for right now
                    ulong timeToNextTask = _TasksList[i].NextExecutionTime - now;
                    if (nextTaskIn > timeToNextTask)
                    {
                        nextTaskIn = timeToNextTask;
                    }
                    break;
                }
            }
            if (taskToExecuteCount > 0)
                ReorderTaskList();
        }
        CreatorSemaphore_Release(_TaskListLock, 1);

        //execute selected tasks
        uint i;
        for (i = 0; i < taskToExecuteCount && !_TerminateTaskThread; i++)
        {
            Creator_Log(CreatorLogLevel_Debug, "Executing task %u", pTasksToExecute[i].TaskID);
            CreatorThread_ClearLastError();
            pTasksToExecute[i].TaskCallback(pTasksToExecute[i].TaskID, pTasksToExecute[i].TaskContext);
            Creator_Log(CreatorLogLevel_Debug, "Done executing task %u", pTasksToExecute[i].TaskID);
        }

        if (!_TerminateTaskThread)
        {
            //sleep thread until next task start
            CreatorSemaphore_WaitFor(_TaskWait, 1, nextTaskIn * 1000);
            //CreatorThread_Sleep(_TaskRunnerThread, nextTaskIn);
        }
    }
}

static void ReorderTaskList()
{
    uint index;
    uint newTaskCount = 0;
    for (index = 0; index < _TasksCount; index++)
    {
        //position of the task to be executed soonest, -1 for unknown
        int minPos = (_TasksList[index].TaskCallback) ? (int)index : -1;
        uint j;
        for (j = index + 1; j < _TasksCount; j++)
        {
            if (!_TasksList[j].TaskCallback)
            {
                continue;
            }
            if (minPos == -1)
            {
                minPos = (int)j;
            }
            else if (_TasksList[j].NextExecutionTime < _TasksList[minPos].NextExecutionTime)
            {
                minPos = (int)j;
            }
        }

        if (minPos != -1)
        {
            //either the current element is valid, or there is an element that will take its place
            newTaskCount++;
        }

        if (minPos > (int)index)
        {
            //swap elements
            CreatorSchedulerTask tmpTask = _TasksList[minPos];
            _TasksList[minPos] = _TasksList[index];
            _TasksList[index] = tmpTask;
        }
    }
    _TasksCount = newTaskCount;
}

