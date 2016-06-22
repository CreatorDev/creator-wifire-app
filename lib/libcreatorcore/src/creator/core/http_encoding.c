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

#include <stdio.h>
#include <strings.h>

#include "creator/core/creator_debug.h"

#include "creator/core/http_encoding.h"

bool CreatorHTTP_EncodePathSegmentRFC3986(const char *value, char *dest, size_t destSize, size_t *encodedSize)
{
	Creator_Assert(dest || encodedSize, "Neither dest or encodedSize are non-NULL");
	Creator_Assert(dest == NULL || destSize > 0, "Invalid buffer size");
	Creator_Assert(value != NULL, "Invalid source data");
	if (!(dest || encodedSize) || !(dest == NULL || destSize > 0) || !(value != NULL))
	{
		return false;
	}

	const char *currentSourcePosition = value;
	char *currentDestinationPosition = dest;
	size_t destAvailable = destSize;
	bool bSuccess = true;

	if (encodedSize)
	{
		*encodedSize = 0;
	}

	while (bSuccess && *currentSourcePosition != '\0')
	{
		size_t encodedCharSize = 0;
		char currentChar = *currentSourcePosition;
		switch (currentChar)
		{
			//sub-delims
			case '!': case '$': case '&': case '\'': case '(': case ')':
			case '*': case '+': case ',': case ';': case '=':
			//FIXME * and & make the server explode

			//unreserved
			case '-': case '.': case '_': case '~':
			case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
			case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
			case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
			case 's': case 't': case 'u': case 'v': case 'w': case 'x':
			case 'y': case 'z':
			case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
			case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
			case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
			case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
			case 'Y': case 'Z':
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':

			//pchar
			case ':':
			case '@':
				encodedCharSize = 1;
				if (currentDestinationPosition && destAvailable > encodedCharSize) {
					*currentDestinationPosition = currentChar;
				}
				break;

			default:
				encodedCharSize = 3;
				if (currentDestinationPosition && destAvailable > encodedCharSize) {
					snprintf(currentDestinationPosition, 4, "%%%02X", currentChar);
				}
				break;
		}

		if (encodedSize)
		{
			*encodedSize += encodedCharSize;
		}
		if (currentDestinationPosition)
		{
			if (destAvailable >= encodedCharSize)
			{
				currentDestinationPosition += encodedCharSize;
				destAvailable -= encodedCharSize;
			}
			else
			{
				bSuccess = false;
			}
		}
		currentSourcePosition++;
	}
	if (!bSuccess)
	{
		//hide the stuff we tried to write so far
		currentDestinationPosition = dest;

		if (encodedSize)
		{
			*encodedSize = 0;
		}
	}
	return bSuccess;
}

enum decodeState
{
	stateNormal,
	stateFoundPercent,
	stateFoundFirstDigit,
	stateWriteAccumulator,

	stateEnd,

	stateBroken
};

bool CreatorHTTP_DecodePathSegmentRFC3986(const char *value, char *dest, size_t destSize, size_t *decodedSize)
{
	Creator_Assert(dest || decodedSize, "Neither dest or decodedSize are non-NULL");
	Creator_Assert(dest == NULL || destSize > 0, "Invalid buffer size");
	Creator_Assert(value != NULL, "Invalid source data");
	if (!(dest || decodedSize) || !(dest == NULL || destSize > 0) || !(value != NULL))
	{
		return false;
	}

	const char *currentSourcePosition = value;
	char *currentDestinationPosition = dest;
	size_t destAvailable = destSize;

	if (decodedSize)
	{
		*decodedSize = 0;
	}

	enum decodeState state = stateNormal;
	char encodedAccumulator = 0;

	while (state != stateBroken && state != stateEnd)
	{
		char currentChar = *currentSourcePosition;
		switch (currentChar)
		{
			case '?': case '/': case '\0':
				if (state == stateNormal)
				{
					state = stateEnd;
				}
				else {
					state = stateBroken;
				}
				break;

			//depending on state (decoding %XX), numbers will either be decoded or kept as such
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
			{
				if (state == stateFoundPercent)
				{
					encodedAccumulator = (currentChar - '0') * 10;
					state = stateFoundFirstDigit;
					break;
				}
				else if (state == stateFoundFirstDigit)
				{
					encodedAccumulator += (currentChar - '0');
					state = stateWriteAccumulator;
					break;
				}
				else {
					//fall-through, normal copy
				}
			}

			//sub-delims
			case '!': case '$': case '&': case '\'': case '(': case ')':
			case '*': case '+': case ',': case ';': case '=':
			//FIXME * and & make the server explode

			//unreserved
			case '-': case '.': case '_': case '~':
			case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
			case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
			case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
			case 's': case 't': case 'u': case 'v': case 'w': case 'x':
			case 'y': case 'z':
			case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
			case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
			case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
			case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
			case 'Y': case 'Z':

			//pchar
			case ':':
			case '@':
				if (!state == stateNormal)
				{
					state = stateBroken;
				}
				break;

			case '%':
				if (state == stateNormal)
				{
					state = stateFoundPercent;
				}
				else
				{
					state = stateBroken;
				}
				break;

			default:
				state = stateBroken;
				break;
		}
		if (state == stateWriteAccumulator)
		{
			currentChar = encodedAccumulator;
			encodedAccumulator = 0;
			state = stateNormal;
		}
		if (state == stateNormal)
		{
			if (decodedSize)
			{
				(*decodedSize)++;
			}
			if (currentDestinationPosition)
			{
				if (destAvailable <= 1)
				{
					state = stateBroken;
				}
				else
				{
					*currentDestinationPosition = currentChar;
					currentDestinationPosition++;
					destAvailable--;
				}
			}
			currentSourcePosition++;
		}
	}
	if (state != stateEnd)
	{
		//hide the stuff we tried to write so far
		currentDestinationPosition = dest;

		if (decodedSize)
		{
			*decodedSize = 0;
		}
	}
	return state == stateEnd;
}

