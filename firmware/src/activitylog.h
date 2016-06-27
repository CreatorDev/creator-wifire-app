/***********************************************************************************************************************
 Copyright (c) 2016, Imagination Technologies Limited
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

#ifndef APP_ACTIVITYLOG_H_
#define APP_ACTIVITYLOG_H_

#include <stdbool.h>
#include <stdarg.h>
#include "creator/core/base_types.h"

#define ACTIVITYLOG_MESSAGE_LENGTH	(120)
#define	CREATOR_ACTIVITY_ERRORCODE_NONE	0

typedef enum
{
    CreatorActivityLogLevel_None = 0,
    CreatorActivityLogLevel_Error,
    CreatorActivityLogLevel_Warning,
    CreatorActivityLogLevel_Information,
    CreatorActivityLogLevel_Debug,
    CreatorActivityLogLevel_Max
} CreatorActivityLogLevel;

typedef enum
{
    CreatorActivityLogCategory_HardwareBoot = 0,
    CreatorActivityLogCategory_Startup,
    CreatorActivityLogCategory_SystemRuntime,
    CreatorActivityLogCategory_App,
    CreatorActivityLogCategory_Shutdown,
    CreatorActivityLogCategory_Max
} CreatorActivityLogCategory;

typedef struct
{
    uint32 Timestamp;			// Epoch seconds since Jan 1 1970
    uint8 Category;
    uint8 LogLevel;
    uint16 ErrorCode;
    char Message[ACTIVITYLOG_MESSAGE_LENGTH];	// null terminated
} ActivityLog;

typedef struct
{
    int RequestCount;		// maximum ActivityLog records to read
    int NextIndex;		// next index to read (modified by read), -1 = start from first
    bool ReadDescending;		// true = descending: read newest first, false = ascending: read oldest first
    ActivityLog *ReadLog;		// must have enough room for requestCount records
} ActivityLogRequest;

/**
 * \brief Set ActivityLog system-debug logging mode.
 */
void Creator_ActivityLogSystemMode(CreatorActivityLogCategory category);

/**
 * \brief Logs an activity message. Activity messages are stored if logging is enabled for the level and category.
 *
 * @param level importance ofs the message
 * @param category of the message
 * @param szMessage printf-formatted message
 *
 * @return true if message was stored in the activity log
 */
void Creator_ActivityLogWrite(CreatorActivityLogLevel level, CreatorActivityLogCategory category, int errorCode, char *szMessage, ...);

#define CreatorActivity_Log Creator_ActivityLogWrite

#endif /* APP_ACTIVITYLOG_H_ */
