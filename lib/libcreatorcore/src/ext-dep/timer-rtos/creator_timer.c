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

#include "creator/core/creator_timer.h"
#include "creator/core/creator_task_scheduler.h"
#include "creator/core/creator_memalloc.h"

#include "FreeRTOS.h"
#include "timers.h"

typedef struct TimerInfoImpl
{
    CreatorTimer_Callback Runnable;
    void * Context;
    TimerHandle_t Timer;
}*TimerInfo;

static uint _TicksPerSecond = 1000;

static void FireTimerCallBack(CreatorTaskID taskID, void *context)
{
    TimerInfo timerInfo = (TimerInfo)context;
    if (timerInfo)
    {
        if(timerInfo->Runnable)
        {
            timerInfo->Runnable((CreatorTimer)timerInfo, timerInfo->Context);
        }
    }
}

static void TimerCallBack(xTimerHandle timerHandle)
{
    TimerInfo timerInfo = (TimerInfo) pvTimerGetTimerID(timerHandle);
    if (timerInfo)
    {
        CreatorScheduler_ScheduleTask(FireTimerCallBack, timerInfo, 0, false);
    }
}

void CreatorTimer_Free(CreatorTimer *self)
{
    if (self && *self)
    {
        CreatorTimer timer = *self;
        CreatorTimer_Stop(timer);
        TimerInfo timerInfo = (TimerInfo)timer;
        xTimerDelete(timerInfo->Timer,0);
        Creator_MemFree((void **)self);
    }
}

uint CreatorTimer_GetTickCount()
{
    return xTaskGetTickCount();
}

uint CreatorTimer_GetTicksPerSecond()
{
    return _TicksPerSecond;
}

void CreatorTimer_Initialise(void)
{

}

CreatorTimer CreatorTimer_New(const char *name, uint periodInMilliseconds, bool continuous, CreatorTimer_Callback runnable, void *context)
{
    TimerInfo timerInfo = (TimerInfo)Creator_MemAlloc(sizeof(struct TimerInfoImpl));
    if (!timerInfo)
    {
        return NULL;
    }
    UBaseType_t reload;
    if (continuous)
    reload = pdTRUE;
    else
    reload = pdFALSE;
    timerInfo->Runnable = runnable;
    timerInfo->Context = context;

    timerInfo->Timer = xTimerCreate(name, (periodInMilliseconds * CreatorTimer_GetTicksPerSecond())/1000, reload, (void*)timerInfo, TimerCallBack);

    return (CreatorTimer)timerInfo;
}

bool CreatorTimer_Reset(CreatorTimer self)
{
    bool result = false;
    if (self)
    {
        TimerInfo timerInfo = (TimerInfo)self;
        result = (xTimerReset(timerInfo->Timer, 0) == pdPASS);
    }
    return result;
}

bool CreatorTimer_SetPeriod(CreatorTimer self, uint periodInMilliseconds)
{
    bool result = false;
    if (self)
    {
        TimerInfo timerInfo = (TimerInfo)self;
        result = (xTimerChangePeriod(timerInfo->Timer,(periodInMilliseconds * CreatorTimer_GetTicksPerSecond())/1000, 0) == pdPASS);
    }
    return result;
}

void CreatorTimer_SetTicksPerSecond(uint ticks)
{
    _TicksPerSecond = ticks;
}

void CreatorTimer_Shutdown(void)
{

}

bool CreatorTimer_Start(CreatorTimer self)
{
    bool result = false;
    if (self)
    {
        TimerInfo timerInfo = (TimerInfo)self;
        result = (xTimerStart(timerInfo->Timer, 0) == pdPASS);
    }
    return result;
}

bool CreatorTimer_Stop(CreatorTimer self)
{
    bool result = false;
    if (self)
    {
        TimerInfo timerInfo = (TimerInfo)self;
        result = (xTimerStop(timerInfo->Timer, 0) == pdPASS);
    }
    return result;
}

#endif
