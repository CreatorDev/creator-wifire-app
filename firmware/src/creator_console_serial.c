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

/*
 * CreatorConsole implementation using CreatorSerial
 *
 */


#include "creator/creator_console.h"

#include "creator/creator_serial.h"
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

static CreatorSerialDevice serialHandle;
static bool _ConsoleInitialised = false;

#define CreatorConsole_UART			4
#define CreatorConsole_Baud			115200

#define CREATOR_CONSOLE_BUFFER_LEN		1024

bool CreatorConsole_Init()
{
    serialHandle = CreatorSerial_Init(CreatorConsole_UART, CreatorConsole_Baud);
    _ConsoleInitialised = true;
    return true;
}

void CreatorConsole_Printf(const char* format, ...)
{
    if (_ConsoleInitialised)
    {
        va_list arg_list;
        char buff[CREATOR_CONSOLE_BUFFER_LEN];

        va_start(arg_list, format);
        vsnprintf(buff, CREATOR_CONSOLE_BUFFER_LEN, format, arg_list);

        CreatorSerial_Puts(serialHandle, buff);

        va_end(arg_list);
    }
}

void CreatorConsole_Puts(const char* msg)
{
    if (_ConsoleInitialised)
    {
        CreatorSerial_Puts(serialHandle, msg);
    }
}

bool CreatorConsole_Ready()
{
    bool result = false;
    if (_ConsoleInitialised)
    {
        result = CreatorSerial_Ready(serialHandle);
    }
    return result;
}

char CreatorConsole_Getc()
{
    char result = '\0';
    if (_ConsoleInitialised)
    {
        result = CreatorSerial_Getc(serialHandle);
    }
    return result;
}

void CreatorConsole_Putc(char c)
{
    if (_ConsoleInitialised)
    {
        CreatorSerial_Putc(serialHandle, c);
    }
}

int CreatorConsole_Getline(char* line, int bufferLen)
{
    int result = 0;
    if (_ConsoleInitialised)
    {
        result = CreatorSerial_Getline(serialHandle, line, bufferLen);
    }
    return result;
}

int CreatorConsole_Scanf(const char *format, ...)
{
    int result = 0;
    if (_ConsoleInitialised)
    {
        // no supported way to pass on variadic args, so this code gets repeated here
        int i = 0;
        char buff[CREATOR_CONSOLE_BUFFER_LEN];
        va_list arg_list;
        va_start(arg_list, format);
        while (i < CREATOR_CONSOLE_BUFFER_LEN)
        {
            buff[i] = CreatorSerial_Getc(serialHandle);
            CreatorSerial_Putc(serialHandle, buff[i]);
            if (buff[i] == '\n' || buff[i] == '\r')
                break;
            i++;
        }
        result = vsscanf(buff, format, arg_list);
        va_end(arg_list);
    }
    return result;

}

//These are duplicates of the above functions for deprecation.

void SYS_CONSOLE_MESSAGE(const char * message)
{
    if (_ConsoleInitialised)
    {
        CreatorSerial_Puts(serialHandle, message);
    }
}

void SYS_CONSOLE_PRINT(const char* format, ...)
{
    if (_ConsoleInitialised)
    {
        va_list arg_list;
        char buff[CREATOR_CONSOLE_BUFFER_LEN];

        va_start(arg_list, format);
        vsnprintf(buff, CREATOR_CONSOLE_BUFFER_LEN, format, arg_list);

        CreatorSerial_Puts(serialHandle, buff);

        va_end(arg_list);
    }
}

