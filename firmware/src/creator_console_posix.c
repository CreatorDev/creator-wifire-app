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
 * CreatorConsole implementation using posix stdout
 *
 */

#include "creator/creator_console.h"

#include <sys/poll.h>
#include <termios.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define CREATOR_CONSOLE_BUFFER_LEN		1024

bool CreatorConsole_Init()
{
	bool sucess = false;

	// disable canonical mode using termios (man 3 termios)
	// this is a POSIX (POSIX.1-2001) way to disable line buffering
	struct termios tty_attr;
	if (tcgetattr(0, &tty_attr) == 0)
	{
		tty_attr.c_lflag &= ~ICANON;

		/* to make CreatorConsole identical on WiFire and posix we could disable echo
		 * then re-enable for scanf. Currently code that uses getchar et.al. has
		 * platform specific workarounds for this */
		//tty_attr.c_lflag &= ~ECHO;

		if (tcsetattr(0, 0, &tty_attr) == 0)
			sucess = true;
	}

	if (!sucess)
	{
		// fallback for ci20
		if (system("stty -icanon") == 0)
			sucess = true;
	}

	return sucess;
}

void CreatorConsole_Printf(const char* format, ...)
{
	va_list arg_list;
	char buff[CREATOR_CONSOLE_BUFFER_LEN];

	va_start(arg_list, format);
	vsnprintf(buff, CREATOR_CONSOLE_BUFFER_LEN, format, arg_list);

	fputs(buff, stdout);
	fflush(stdout);

	va_end(arg_list);
}

void CreatorConsole_Puts(const char* msg)
{
	fputs(msg, stdout);
	fflush(stdout);
}

bool CreatorConsole_Ready()
{
	struct pollfd fds;
	fds.fd = 0;
	fds.events = POLLIN;
	return poll(&fds, 1, 0) > 0;
}

char CreatorConsole_Getc()
{
	return getchar();
}

void CreatorConsole_Putc(char c)
{
	putchar(c);
	fflush(stdout);
}

int CreatorConsole_Getline(char* line, int bufferLen)
{
	fgets(line, bufferLen, stdin);
	return strnlen(line, bufferLen);
}

int CreatorConsole_Scanf(const char *format, ...)
{
	int rc;

	va_list args;
	va_start(args, format);
	rc = vscanf(format, args);
	va_end(args);

	return rc;
}

//These are duplicates of the above functions for deprecation.

void SYS_CONSOLE_MESSAGE(const char * message)
{
	fputs(message, stdout);
	fflush(stdout);
}

void SYS_CONSOLE_PRINT(const char* format, ...)
{
	va_list arg_list;
	char buff[CREATOR_CONSOLE_BUFFER_LEN];

	va_start(arg_list, format);
	vsnprintf(buff, CREATOR_CONSOLE_BUFFER_LEN, format, arg_list);

	fputs(buff, stdout);
	fflush(stdout);

	va_end(arg_list);
}


