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

#include <stddef.h>

#include "creator/core/creator_debug.h"

#include "creator/core/query_encoding.h"

bool CreatorEncodeLuceneQuery(const char *src, char *dest, size_t *destSize)
{
	Creator_Assert(src != NULL, "src must be non-NULL");
	if (src == NULL)
	{
		return false;
	}

	const char *currentSourceChar = src;
	char *currentDestinationChar = dest;
	bool bSuccess = true;

	if (destSize)
	{
		*destSize = 0;
	}

	while (bSuccess && *currentSourceChar != '\0')
	{
		size_t encodedCharSize = 0;
		char curChar = *currentSourceChar;
		switch (curChar)
		{
			case '+': case '-': case '!': case '(': case ')':
			case '{': case '}': case '[': case ']': case '^':
			case '"': case '~': case '*': case '?': case ':':
			case '\\':
			//these last two only need to be escaped when they are paired, i.e. && or ||.
			//Lucene doesn't document how to escape them however, and escaping them always doesn't hurt either
			case '&': case '|':
				//escape with '\'
				if (currentDestinationChar)
				{
					*currentDestinationChar = '\\';
					*(currentDestinationChar+1) = curChar;
				}
				encodedCharSize = 2;
				break;

			default:
				if (currentDestinationChar)
				{
					*currentDestinationChar = curChar;
				}
				encodedCharSize = 1;
				break;
		}

		if (destSize)
		{
			*destSize += encodedCharSize;
		}
		if (currentDestinationChar)
		{
			currentDestinationChar += encodedCharSize;
		}
		currentSourceChar++;
	}
	if (!bSuccess)
	{
		//hide the stuff we tried to write so far
		currentDestinationChar = dest;

		if (destSize)
		{
			*destSize = 0;
		}
	}
	return bSuccess;
}

bool CreatorEncodeQuery(const char *src, char *dest, size_t *destSize)
{
	Creator_Assert(src != NULL, "src must be non-NULL");
	if (src == NULL)
	{
		return false;
	}

	const char *currentSourceChar = src;
	char *currentDestinationChar = dest;
	bool bSuccess = true;

	if (destSize)
	{
		*destSize = 0;
	}

	while (bSuccess && *currentSourceChar != '\0')
	{
		size_t encodedCharSize = 0;
		char curChar = *currentSourceChar;
		switch (curChar)
		{
			case '\\':
			case '\'':
				//escape with '\'
				if (currentDestinationChar)
				{
					*currentDestinationChar = '\\';
					*(currentDestinationChar+1) = curChar;
				}
				encodedCharSize = 2;
				break;

			case '%':
				//escape with '%'
				if (currentDestinationChar)
				{
					*currentDestinationChar = '%';
					*(currentDestinationChar+1) = curChar;
				}
				encodedCharSize = 2;
				break;

			default:
				if (currentDestinationChar)
				{
					*currentDestinationChar = curChar;
				}
				encodedCharSize = 1;
				break;
		}

		if (destSize)
		{
			*destSize += encodedCharSize;
		}
		if (currentDestinationChar)
		{
			currentDestinationChar += encodedCharSize;
		}
		currentSourceChar++;
	}
	if (!bSuccess)
	{
		//hide the stuff we tried to write so far
		currentDestinationChar = dest;

		if (destSize)
		{
			*destSize = 0;
		}
	}
	return bSuccess;
}

