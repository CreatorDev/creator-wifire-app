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
#include <string.h>
#include <stdio.h>

#include "string_manip.h"

#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_debug.h"

char *StringManip_BuildQueryString(const char *szId, const char **valuesArray, size_t valuesCount, const char *valueBinder,
        const char *szTemplate, StringManip_EncodingFn valueEncoder)
{
    if (!szId || !valuesArray || !valueBinder || !szTemplate || !valuesCount)
    {
        return NULL;
    }
    if (!valueEncoder)
    {
        valueEncoder = StringManip_DontEncode;
    }

    bool bNoError = true;

    size_t escapedIdLength;
    bNoError &= valueEncoder(szId, NULL, &escapedIdLength);
    if (escapedIdLength < 3)
    {
        //if the id is smaller than the placeholder, we would overrun our buffer while replacing the value
        escapedIdLength = 3;
    }
    size_t binderLength = strlen(valueBinder);
    //length of template - length of !id and !val placeholders
    size_t templateLength = strlen(szTemplate);
    size_t templatePaddingLength = templateLength - 7;

    char *szResult = NULL;

    //escape values and estimate the size of the result string
    size_t i, totalQueryLength = binderLength * (valuesCount - 1) + 1;
    for (i = 0; i < valuesCount; i++)
    {
        size_t encodedValueLength;
        bNoError &= valueEncoder(valuesArray[i], NULL, &encodedValueLength);
        if (encodedValueLength < 4)
        {
            //if the encoded value is smaller than the placeholder, we would overrun our buffer while replacing the id
            encodedValueLength = 4;
        }

        if (escapedIdLength + encodedValueLength + templatePaddingLength > templateLength)
        {
            totalQueryLength += escapedIdLength + encodedValueLength + templatePaddingLength;
        }
        else
        {
            //the template will be copied first, so it must be able to hold it entirely
            totalQueryLength += templateLength;
        }
    }

    if (bNoError)
    {
        //build the result string, which will first hold the template then have placeholders replaced
        char *szQueryString = Creator_MemAlloc(totalQueryLength);
        if (szQueryString)
        {
            size_t writtenBytes = 0;
            for (i = 0; i < valuesCount; i++)
            {
                //write the value binder
                if (writtenBytes > 0)
                {
                    strcpy(szQueryString + writtenBytes, valueBinder);
                    writtenBytes += binderLength;
                }

                //copy the template into the final buffer
                size_t finalizedBytes = writtenBytes;
                strcpy(szQueryString + finalizedBytes, szTemplate);
                writtenBytes += templateLength;
                //then replace placeholders with encoded values
                StringManip_InplaceReplace(szQueryString + finalizedBytes, &writtenBytes, "!id", szId, valueEncoder);
                StringManip_InplaceReplace(szQueryString + finalizedBytes, &writtenBytes, "!val", valuesArray[i], valueEncoder);
            }
            *(szQueryString + writtenBytes) = '\0';
            szResult = szQueryString;
        }
    }

    return szResult;
}

