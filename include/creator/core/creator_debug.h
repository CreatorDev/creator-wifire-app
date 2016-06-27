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


#ifndef CREATOR_DEBUG_H_
#define CREATOR_DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef enum
{
	CreatorLogLevel_None,
	CreatorLogLevel_Error,
	CreatorLogLevel_Warning,
	CreatorLogLevel_Info,
	CreatorLogLevel_Debug,
        CreatorLogLevel_Max
} CreatorLogLevel;

/**
 * \brief Initializes the log library.
 *
 * Called by \ref CreatorCore_Initialise.
 *
 * @return
 */
bool CreatorLog_Initialise(void);

/**
 * \brief Frees the log library.
 *
 * Called by \ref CreatorCore_Shutdown.
 */
void CreatorLog_Shutdown(void);

/**
 * \brief Set logging level.
 *
 * @param level to log (messages with a higher level can be ignored)
 */
void CreatorLog_SetLevel(CreatorLogLevel level);

#ifdef CREATOR_DEBUG_ON

#include <stdarg.h>
/**
 * \brief Called to check that no programmation error was encountered.
 *
 * @param assertion 0 if there is an error, non-0 otherwise
 * @param message printf-formatted message
 */
void Creator_Assert(int assertion, const char *message, ...);

/**
 * \brief Logs a message.
 *
 * @param level importance of the message
 * @param message printf-formatted message
 */
void Creator_Log(CreatorLogLevel level, const char *message, ...);
//for internal calls from Creator_Assert to CreatorLog
void Creator_Logv(CreatorLogLevel level, const char *message, va_list vl);
void Creator_LogvRaw(CreatorLogLevel level, const char *message, va_list vl);
void Creator_LogMessage(CreatorLogLevel level, const char *message);

#else

#define Creator_Assert(assertion, message, ...) 	while(0) {}
#define Creator_Log(level, message, ...) 			while(0) {}
#define Creator_Logv(level, message, vl) 			while(0) {}
#define Creator_LogvRaw(level, message, vl)        while(0) {}
#define Creator_LogMessage(level, message)		while(0) {}

#endif



#ifdef __cplusplus
}
#endif

#endif /* CREATOR_DEBUG_H_ */
