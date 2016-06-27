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

#include <string.h>

#include "string_manip.h"
#include "creator/core/creator_memalloc.h"


char *StringManip_EscapeCharacters(const char *src, const char *szEscapableCharacters, const char escapeCharacter)
{
    if (!src || !szEscapableCharacters)
    {
        return NULL;
    }
    size_t srcLength = strlen(src);
    size_t dstLength = srcLength;
    char *sEscapePositions = Creator_MemAlloc(srcLength);
    memset(sEscapePositions, 0, srcLength);

    char *pEscapePos = sEscapePositions;
    const char *sourcePosition = src;
    while (*sourcePosition)
    {
        const char *pEscapable = szEscapableCharacters;
        while (*pEscapable)
        {
            if (*pEscapable == *sourcePosition)
            {
                *pEscapePos = 1;
                dstLength++;
                break;
            }
            pEscapable++;
        }
        sourcePosition++;
        pEscapePos++;
    }

    char *szResult = NULL;
    if (dstLength > srcLength)
    {
        szResult = Creator_MemAlloc(dstLength + 1);

        char *pResult = szResult;
        pEscapePos = sEscapePositions;
        sourcePosition = src;
        while (*sourcePosition)
        {
            if (*pEscapePos)
            {
                *pResult++ = escapeCharacter;
            }
            *pResult++ = *sourcePosition++;
            pEscapePos++;
        }
        szResult[dstLength] = '\0';
    }
    else
    {
        szResult = Creator_MemAlloc(dstLength + 1);
        strcpy(szResult, src);
    }

    Creator_MemFree((void **)&sEscapePositions);
    return szResult;
}
