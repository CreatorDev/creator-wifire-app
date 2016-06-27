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

#ifdef MICROCHIP_PIC32

/*! \file creator_time.c
 *  \brief LibCreatorCore .
 */

#include "creator/core/creator_debug.h"
#include "creator/core/creator_time.h"
#include "creator/core/creator_timer.h"
#include "creator/core/creator_memalloc.h"
#include "peripheral/int/plib_int.h"
#include "peripheral/rtcc/plib_rtcc.h"
#include "creator/core/base_types.h"
#include "creator/core/creator_task_scheduler.h"
#include "creator/core/timeparse.h"

// TODO - remove server sync dependence
#define SERVERTIME_RESYNC_DELAY_DEFAULT (15*60)
#define SERVERTIME_RESYNC_DELAY_MAX		(4*60*60)
volatile time_t SERVERTIME_RESYNC_DELAY = SERVERTIME_RESYNC_DELAY_MAX;

#define IS_TIME_UPDATE_IN_RANGE(time_interval)	(time_interval > (SERVERTIME_RESYNC_DELAY - (SERVERTIME_RESYNC_DELAY >> 4))) && (time_interval < (SERVERTIME_RESYNC_DELAY + (SERVERTIME_RESYNC_DELAY >> 4)))
#define CORRECT_AVG_SIZE	4

static void TimeCorrectTask(CreatorTaskID taskID, void *clientArg);
//in seconds

static time_t totalCorrected = 0;
static time_t errorTime = 0;
static time_t lastUpdated = 0;
static time_t correctPrev[CORRECT_AVG_SIZE] = {0};
static uint8 correctPos = 0;
static uint8 correctSize = 0;

static CreatorTaskID _CorrectTask = 0;

uint16 BCDToDecimal(uint16 val)
{
  return ( (val/16*10) + (val%16) );
}

time_t Creator_GetTime(time_t *time)
{
    time_t result;
    struct tm convertedTime;

    uint currentDate;
    uint currentTime;
    uint reReadDate;

	while(PLIB_RTCC_RTCSyncStatusGet(RTCC_ID_0)) Nop();

    do
    {
        currentDate = PLIB_RTCC_RTCDateGet(RTCC_ID_0);
        currentTime =  PLIB_RTCC_RTCTimeGet(RTCC_ID_0);
        reReadDate = PLIB_RTCC_RTCDateGet(RTCC_ID_0);
    } while (currentDate != reReadDate);

    currentTime=currentTime >>8;
    convertedTime.tm_sec = BCDToDecimal(currentTime & 0xff);
    currentTime=currentTime >>8;
    convertedTime.tm_min = BCDToDecimal(currentTime & 0xff);
    currentTime=currentTime >>8;
    convertedTime.tm_hour = BCDToDecimal(currentTime & 0xff);

    currentDate=currentDate >>8;
    convertedTime.tm_mday = BCDToDecimal(currentDate & 0xff);
    currentDate=currentDate >>8;
    convertedTime.tm_mon = BCDToDecimal(currentDate & 0xff) - 1;
    currentDate=currentDate >>8;
    convertedTime.tm_year = BCDToDecimal(currentDate & 0xff) + 100;
    convertedTime.tm_isdst = 0;
    result = mktime(&convertedTime);
    if (time)
    {
        *time = result;
	}
    return result;
}


void Creator_UpdateTime(time_t time)
{
    struct tm convertedTime;
    struct tm *gtm = gmtime_r(&time, &convertedTime);
    if (gtm)
    {
        int year =(convertedTime.tm_year-100);
        int month = (convertedTime.tm_mon+1);
        int day = (convertedTime.tm_mday);

        uint newDate =
        (0x10000000 * (year/10)) + (0x01000000 *(year % 10)) +
        (0x00100000 * (month /10))+ (0x00010000 *(month % 10)) +
        (0x00001000 * (day/10)) + (0x00000100 *(day % 10)) +
        (convertedTime.tm_wday);

        uint newTime =
        (0x10000000 * (convertedTime.tm_hour/10))+ (0x01000000 *(convertedTime.tm_hour % 10)) +
        (0x00100000 * (convertedTime.tm_min/10)) + (0x00010000 *(convertedTime.tm_min % 10)) +
        (0x00001000 * (convertedTime.tm_sec/10)) + (0x00000100 *(convertedTime.tm_sec % 10));

//		if(!PLIB_OSC_SecondaryIsReady(OSC_ID_0))
//		{
//			Creator_Log(CreatorLogLevel_Info, "SOSC not ready!!!");
//			while(1);
//		}

        while(PLIB_RTCC_RTCSyncStatusGet(RTCC_ID_0)) Nop();

//		PLIB_RTCC_ClockSourceSelect(RTCC_ID_0, RTCC_CLOCK_SOURCE_SOSC);

        PLIB_INT_Disable(INT_ID_0);
        SYSKEY = 0;
        SYSKEY = 0xaa996655;// Write first unlock key to SYSKEY
        SYSKEY = 0x556699aa;// Write second unlock key to SYSKEY
        //PLIB_CORE_SysUnlock();

        PLIB_RTCC_WriteEnable(RTCC_ID_0);
        PLIB_RTCC_Disable(RTCC_ID_0);

        //while (PLIB_RTCC_ClockRunningStatus(RTCC_ID_0));
        PLIB_RTCC_RTCTimeSet(RTCC_ID_0,newTime);
        PLIB_RTCC_RTCDateSet(RTCC_ID_0,newDate);

//        PLIB_RTCC_ClockSourceSelect (RTCC_ID_0, RTCC_CLOCK_SOURCE_LPRC);
//        PLIB_RTCC_ClockOutputEnable (RTCC_ID_0);

        PLIB_INT_Enable(INT_ID_0);
        PLIB_RTCC_Enable(RTCC_ID_0);

        PLIB_RTCC_WriteDisable(RTCC_ID_0);
        SYSKEY = 0;

    }
}

void Creator_SetTime(time_t time)
{
    time_t current_GetTime = 0;
    time_t correctInterval = 0;
    time_t updateInterval = 0;

    current_GetTime = Creator_GetTime(NULL);
    errorTime = (current_GetTime - time) + totalCorrected;
    updateInterval = lastUpdated ? time - lastUpdated : 0;
    correctInterval = updateInterval && errorTime ? abs(updateInterval / errorTime) : 0;

    if(IS_TIME_UPDATE_IN_RANGE(updateInterval))
    {
        time_t correctSum = 0;
        uint8 correctI = 0;

        if(correctPos > CORRECT_AVG_SIZE - 1)
        correctPos = 0;

        if(correctInterval < SERVERTIME_RESYNC_DELAY)
        correctPrev[correctPos++] = correctInterval;

        correctSize = correctSize >= CORRECT_AVG_SIZE ? CORRECT_AVG_SIZE : correctPos;

        for(correctI = 0; correctI < correctSize; correctI++)
        correctSum += correctPrev[correctI];

        if(correctSize && correctSum)
        {
            correctInterval = correctSum / (time_t)correctSize;

            Creator_Log(CreatorLogLevel_Debug, "PIC32MZ time sync [%d] diff: %d, error: %d, corrected: %d, sync interval: %d, correct interval: %d", time, (current_GetTime - time), errorTime, totalCorrected, updateInterval, correctInterval);

            if(!_CorrectTask)
            _CorrectTask = CreatorScheduler_ScheduleTask(TimeCorrectTask, NULL, correctInterval, true);
            else
            CreatorScheduler_SetTaskInterval(_CorrectTask, correctInterval);
        }

        if ( abs(current_GetTime - time) < 2 )
        {
            if(SERVERTIME_RESYNC_DELAY < SERVERTIME_RESYNC_DELAY_MAX)
            SERVERTIME_RESYNC_DELAY = SERVERTIME_RESYNC_DELAY << 2;
        }
        else if ( abs(current_GetTime - time) < 5 )
        {
            if(SERVERTIME_RESYNC_DELAY < SERVERTIME_RESYNC_DELAY_MAX)
            SERVERTIME_RESYNC_DELAY = SERVERTIME_RESYNC_DELAY << 1;
        }
        else if ( abs(current_GetTime - time) < 30 )
        {
            if(SERVERTIME_RESYNC_DELAY > SERVERTIME_RESYNC_DELAY_DEFAULT)
            SERVERTIME_RESYNC_DELAY = SERVERTIME_RESYNC_DELAY >> 1;
        }
        else
        {
            SERVERTIME_RESYNC_DELAY = SERVERTIME_RESYNC_DELAY_DEFAULT;
        }
    }
    else
    {
        if(!correctInterval)
        {
            Creator_Log(CreatorLogLevel_Debug, "PIC32MZ time sync reverting to %d interval (expected interval %d got %d)", SERVERTIME_RESYNC_DELAY_DEFAULT, SERVERTIME_RESYNC_DELAY, updateInterval);
        }
        memset(&correctPrev[0], 0, sizeof(correctPrev));
        correctSize = 0;
        correctPos = 0;

        SERVERTIME_RESYNC_DELAY = SERVERTIME_RESYNC_DELAY_DEFAULT;
    }

    totalCorrected = 0;
    lastUpdated = time;

    Creator_UpdateTime(time);
}

static void TimeCorrectTask(CreatorTaskID taskID, void *clientArg)
{
    time_t correctSecond = 0;

    if(errorTime < 0)
    correctSecond = -1;
    else if(errorTime > 0)
    correctSecond = 1;

    Creator_UpdateTime(Creator_GetTime(NULL) - correctSecond);
    totalCorrected += correctSecond;

    //Creator_Log(CreatorLogLevel_Info, "PIC32MZ time correct error %d correction: %d", errorTime, totalCorrected);
}

#endif
