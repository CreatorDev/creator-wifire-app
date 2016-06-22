/************************************************************************************************************************
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
************************************************************************************************************************/


#include <stdio.h>
#include <stdarg.h>

#include "creator/core/creator_debug.h"
#include "lwm2m_debug.h"

static DebugLevel debugLevel = DebugLevel_Error;
static FILE * g_outFile = NULL;

static const char * logLabels[] = { "EMER", "ALERT", "CRIT", "ERROR", "WARN", "NOTICE", "INFO", "DEBUG" };

void Lwm2m_PrintBanner(void)
{
    const char * banner =
    "      _                  _            __  __ ____  __  __ r\n"
    "     / \\__      ____ _  | | __      _|  \\/  |___ \\|  \\/  |r\n"
    "    / _ \\ \\ /\\ / / _` | | | \\ \\ /\\ / / |\\/| | __) | |\\/| |r\n"
    "   / ___ \\ V  V / (_| | | |__\\ V  V /| |  | |/ __/| |  | |r\n"
    "  /_/   \\_\\_/\\_/ \\__,_| |_____\\_/\\_/ |_|  |_|_____|_|  |_|r\n";
    Creator_LogRaw(CreatorLogLevel_Info, "%s\r\n", banner);
    Creator_LogRaw(CreatorLogLevel_Info, ANSI_COLOUR_MAGENTA "   Copyright (C) 2016, Imagination Technologies Limited." ANSI_COLOUR_RESET "\r\n\r\n");
}

//void Lwm2m_Log(DebugLevel level, char const * format, ...)
void Lwm2m_Log(DebugLevel level, const char * fileName, int lineNum, char const * format, ...)
{
    if (level <= debugLevel)
    {
        if ((level >= 0) && (level < (sizeof(logLabels) / sizeof(logLabels[0]))))
        {
            Creator_LogRaw(CreatorLogLevel_None, ANSI_COLOUR_BRIGHT_WHITE " [%s] " ANSI_COLOUR_RESET, logLabels[level]);
            if (debugLevel >= DebugLevel_Debug)
            {
                const char * shortFileName = strrchr(fileName, DIR_SLASH);
                Creator_LogRaw(CreatorLogLevel_None, ANSI_COLOUR_YELLOW "[%s:%d] " ANSI_COLOUR_RESET, shortFileName ? shortFileName+1 : fileName, lineNum);
            }
        }
       
        va_list argp;
        va_start(argp, format);
        Creator_LogvRaw(CreatorLogLevel_None, format, argp);
        va_end(argp);
        Creator_LogMessage(CreatorLogLevel_None, "\r");      // append to \n terminated lines from Awa
    }
}

void Lwm2m_SetOutput(FILE * outFile)
{
}

void Lwm2m_SetLogLevel(DebugLevel level)
{
    debugLevel = level;
}

DebugLevel Lwm2m_GetLogLevel()
{
    return debugLevel;
}

void Lwm2m_SetAwaLogLevel(AwaLogLevel level)
{
    switch (level)
    {
    case AwaLogLevel_None:
        Lwm2m_SetLogLevel(DebugLevel_Emerg);
        break;
    case AwaLogLevel_Error:
        Lwm2m_SetLogLevel(DebugLevel_Error);
        break;
    case AwaLogLevel_Warning:
        Lwm2m_SetLogLevel(DebugLevel_Warning);
        break;
    case AwaLogLevel_Verbose:
        Lwm2m_SetLogLevel(DebugLevel_Info);
        break;
    case AwaLogLevel_Debug:
        Lwm2m_SetLogLevel(DebugLevel_Debug);
        break;
    default:
        Lwm2m_Error("Unknown Awa LogLevel: %d\n", level);
        break;
    }
}
