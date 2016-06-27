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

#include "creator/core/creator_timer.h"
#include "creator/core/creator_memalloc.h"

#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>


typedef struct TimerInfoImpl
{
    CreatorTimer_Callback Runnable;
    void *Context;
    timer_t Timer;
    uint PeriodInMilliseconds;
    bool Continuous;
}*TimerInfo;

struct sigaction *_Action = NULL;

static void TimerCallBack(int sig, siginfo_t *si, void *uc)
{
    if (si)
    {
        TimerInfo timerInfo = (TimerInfo)si->si_value.sival_ptr;
        if (timerInfo)
        {
            if(timerInfo->Runnable)
            {
                timerInfo->Runnable((CreatorTimer)timerInfo, timerInfo->Context);
            }
        }
    }

}

void CreatorTimer_Initialise(void)
{
    if (!_Action)
    {
        _Action = (struct sigaction*)Creator_MemAlloc(sizeof(struct sigaction));
        memset(_Action,0, sizeof(struct sigaction));
        _Action->sa_flags = SA_SIGINFO;
        _Action->sa_sigaction = TimerCallBack;
        sigemptyset(&_Action->sa_mask);
        if (sigaction(SIGRTMIN, _Action, NULL) == -1)
        {
            Creator_MemFree((void **)&_Action);
        }
    }
}

void CreatorTimer_Shutdown(void)
{
    if (_Action)
    {
        Creator_MemFree((void **)&_Action);
    }
}

CreatorTimer CreatorTimer_New(const char *name, uint periodInMilliseconds, bool continuous, CreatorTimer_Callback runnable, void *context)
{
    TimerInfo result = (TimerInfo)Creator_MemAlloc(sizeof(struct TimerInfoImpl));
    if (result)
    {
        if (!_Action)
        {
            CreatorTimer_Initialise();
        }

        result->Runnable = runnable;
        result->Context = context;
        result->PeriodInMilliseconds = periodInMilliseconds;
        result->Continuous = continuous;

        struct sigevent sev;
        memset(&sev,0, sizeof(struct sigevent));
        sev.sigev_notify = SIGEV_SIGNAL;
        sev.sigev_signo = SIGRTMIN;
        sev.sigev_value.sival_ptr = result;
        if (timer_create(CLOCK_REALTIME, &sev, &result->Timer) == -1)
        {

        }
    }
    return (CreatorTimer)result;
}

void SetTimerPeriod(TimerInfo timerInfo, struct itimerspec *timePeriod)
{
    if (timerInfo->Continuous)
    {
        timePeriod->it_interval.tv_sec = timerInfo->PeriodInMilliseconds / 1000;
        timePeriod->it_interval.tv_nsec = ((timerInfo->PeriodInMilliseconds % 1000) * 1000000);
    }
    else
    {
        timePeriod->it_interval.tv_sec = 0;
        timePeriod->it_interval.tv_nsec = 0;
    }
    timePeriod->it_value.tv_sec = timerInfo->PeriodInMilliseconds / 1000;
    timePeriod->it_value.tv_nsec = ((timerInfo->PeriodInMilliseconds % 1000) * 1000000);
}

bool CreatorTimer_Reset(CreatorTimer self)
{
    bool result = false;
    if (self)
    {
        TimerInfo timerInfo = (TimerInfo)self;
        struct itimerspec timePeriod;
        SetTimerPeriod(timerInfo, &timePeriod);
        if (timer_settime(timerInfo->Timer, 0, &timePeriod, NULL) == 0)
        {
            result = true;
        }
    }
    return result;
}

bool CreatorTimer_SetPeriod(CreatorTimer self, uint periodInMilliseconds)
{
    bool result = false;
    if (self)
    {
        TimerInfo timerInfo = (TimerInfo)self;
        timerInfo->PeriodInMilliseconds = periodInMilliseconds;
        struct itimerspec timePeriod;
        SetTimerPeriod(timerInfo, &timePeriod);
        if (timer_settime(timerInfo->Timer, 0, &timePeriod, NULL) == 0)
        {
            result = true;
        }
    }
    return result;
}

bool CreatorTimer_Start(CreatorTimer self)
{
    bool result = false;
    if (self)
    {
        TimerInfo timerInfo = (TimerInfo)self;
        struct itimerspec timePeriod;
        SetTimerPeriod(timerInfo, &timePeriod);
        if (timer_settime(timerInfo->Timer, 0, &timePeriod, NULL) == 0)
        {
            result = true;
        }
    }
    return result;
}

bool CreatorTimer_Stop(CreatorTimer self)
{
    bool result = false;
    if (self)
    {
        TimerInfo timerInfo = (TimerInfo)self;
        struct itimerspec timePeriod;
        SetTimerPeriod(timerInfo, &timePeriod);
        timePeriod.it_value.tv_sec = 0;
        timePeriod.it_value.tv_nsec = 0;
        if (timer_settime(timerInfo->Timer, 0, &timePeriod, NULL) == 0)
        {
            result = true;
        }
    }
    return result;
}

void CreatorTimer_Free(CreatorTimer *self)
{
    if (self && *self)
    {
        CreatorTimer timer = *self;
        CreatorTimer_Stop(timer);
        TimerInfo timerInfo = (TimerInfo)timer;
        timer_delete(timerInfo->Timer);
        Creator_MemFree((void **)self);
    }
}

uint CreatorTimer_GetTickCount()
{
    uint result = 0;
    struct timespec timePeriod;
    if (clock_gettime(CLOCK_MONOTONIC,&timePeriod) == 0)
    {
        result = (uint)((timePeriod.tv_sec * 1000L) + (timePeriod.tv_nsec/1000000));
    }
    return result;
}

uint CreatorTimer_GetTicksPerSecond()
{
    return 1000;
}

void CreatorTimer_SetTicksPerSecond(uint ticks)
{

}

#endif
