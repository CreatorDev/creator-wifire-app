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

/*! \file string_builder.c
 *  \brief APIs for manipulating dynamically growing C-strings
 */

#include "stdio.h"
#include "string.h"
#include "creator/core/creator_memalloc.h"
#include "string_builder.h"

#define     INITIAL_SIZE				(32)
#define     GROWTH_FACTOR				(2)


// Dynamic string implementation datatype
typedef struct
{
        char* Str;					// Pointer to allocated buffer
        unsigned int StrSize;		// dynamically allocated buffer size
        unsigned int Length;		// Portion of the buffer that is utilised
} StringBuilderImpl;


//
// Pointer type
//
typedef StringBuilderImpl* DynamicString;


//
// Public interfaces
//


const char* StringBuilder_GetCString(StringBuilder dynString)
{
	DynamicString string = (DynamicString) dynString;
	if (string && string->Str)
	{
		return (const char*) string->Str;
	}
	return NULL;
}


int StringBuilder_GetLength(StringBuilder dynString)
{
	int result = -1;
	DynamicString string = (DynamicString) dynString;
	if (string)
	{
		result = string->Length;
	}
	return result;
}


StringBuilder StringBuilder_Append(StringBuilder dynString, const char* str)
{
	DynamicString string = (DynamicString) dynString;
	if (string && str)
	{
		unsigned int addedLength = strlen((char*) str);

		if(addedLength == 0)
			return dynString;

		if ((string->Length + addedLength + 1) <= string->StrSize)
		{
			memcpy(string->Str + string->Length, str, sizeof(char) * addedLength+1);	// include the null character from 'str'
			string->Length += addedLength;
		}
		else
		{
			// Scale the buffer
			unsigned iterations = 1;

			while ( (string->StrSize*GROWTH_FACTOR*iterations) <= (string->Length+addedLength+1) )
				iterations++;


			// Grow the string
			unsigned int newStrSize = (GROWTH_FACTOR*iterations) * string->StrSize;
			char *newBuf = (char*) Creator_MemAlloc(sizeof(char) * newStrSize);
			if (newBuf)
			{
				memcpy(newBuf, string->Str, string->StrSize);
				memset(newBuf + string->StrSize, 0, newStrSize - string->StrSize);
				memcpy(newBuf + string->Length, str, addedLength + 1);

				string->StrSize = newStrSize;
				string->Length += addedLength;
				Creator_MemFree((void **) &string->Str);
				string->Str = newBuf;
			}
		}
	}

	return (StringBuilder) string;
}


StringBuilder StringBuilder_AppendInt(StringBuilder dynString, int value)
{
	DynamicString string = (DynamicString) dynString;
	if (string)
	{
		// 32-bits gives a 10-digit number + 1 for the null-terminator
		char str[11];
		memset(str, 0 ,sizeof(str) );
		sprintf((char*) str, (const char*) "%d", value);
		string = StringBuilder_Append(string, str);
	}
	return (StringBuilder) string;
}


StringBuilder StringBuilder_Clear(StringBuilder dynString)
{
	DynamicString string = (DynamicString) dynString;

	if (string)
	{
		if (string->Str)
		{
			memset(string->Str, 0, sizeof(char) * string->StrSize);
			string->Length = 0;
		}
	}
	return string;
}


StringBuilder StringBuilder_New(unsigned int initialSize)
{
	StringBuilder result = 0;
	unsigned int initSize = initialSize ? initialSize : INITIAL_SIZE;

	DynamicString newString = Creator_MemAlloc(sizeof(StringBuilderImpl));
    if (newString)
    {
		newString->Str = Creator_MemAlloc(sizeof(char) * initSize);
		if (newString->Str)
		{
			memset(newString->Str, 0, sizeof(char) * initSize);
			newString->Length = 0;
			newString->StrSize = sizeof(char) * initSize;
			result = (StringBuilder) newString;
		}
		else
		{
			Creator_MemFree((void **) &newString);
		}
	}

	return result;
}


void StringBuilder_Free(StringBuilder *dynString)
{
	DynamicString string = (DynamicString) *dynString;
	if (string && string->Str)
	{
		Creator_MemFree((void **) &string->Str);
		string->Length = 0;
		string->StrSize = 0;

		Creator_MemFree((void **) dynString);
	}
}
