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

#ifndef _POSIX_SOURCE
	#define _POSIX_SOURCE
#endif
#ifndef _DARWIN_C_SOURCE
	#define _DARWIN_C_SOURCE
#endif
#include "creator/core/http_query.h"
#include "creator/core/base_types.h"
#include "creator/core/http_encoding.h"
//#include "creator/core/http_private.h"
//#include "creator/core/creator_memorymanager_methods.h"
#include "creator/core/base_types_methods.h"
#include "creator/core/creator_memalloc.h"
#include "oauth.h"


#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define CREATOR_HTTPQUERYPARAMETER_DEFAULTSIZE 10

typedef struct _CreatorHTTPQueryParameter
{
    char *Key;
    char *Value;
}*CreatorHTTPQueryParameter;

struct _CreatorHTTPQuery
{
    char *BaseUrl;
    uint Size;
    uint Used;
    CreatorHTTPQueryParameter *Parameters;
};

static void CreatorHTTPQuery_AddParameters(CreatorHTTPQuery query, char *key, char *value)
{
    if (query)
    {
        if (query->Used == query->Size)
        {
            CreatorHTTPQueryParameter *newParameters = Creator_MemRealloc(query->Parameters,
                    (query->Size + CREATOR_HTTPQUERYPARAMETER_DEFAULTSIZE) * sizeof(CreatorHTTPQueryParameter));
            if (newParameters)
            {
                query->Size += CREATOR_HTTPQUERYPARAMETER_DEFAULTSIZE;
                query->Parameters = newParameters;
            }
            else
            {
                return;
            }
        }
        query->Parameters[query->Used] = Creator_MemAlloc(sizeof(struct _CreatorHTTPQueryParameter));
        if (query->Parameters[query->Used])
        {
            query->Parameters[query->Used]->Key = key;
            query->Parameters[query->Used]->Value = value;
            query->Used++;
        }
    }
}

bool CreatorHTTPQuery_AppendPathSuffix(CreatorHTTPQuery self, const char *suffix)
{
    if (!self || !self->BaseUrl || !suffix)
    {
        return false;
    }
    size_t urlLength = strlen(self->BaseUrl);
    size_t encodedSuffixLength;
    bool bSuccess = CreatorHTTP_EncodePathSegmentRFC3986(suffix, NULL, 0, &encodedSuffixLength);
    if (bSuccess)
    {
        size_t newUrlLength = urlLength + 1 + encodedSuffixLength + 1;
        char *newUrl = Creator_MemAlloc(newUrlLength);
        if (newUrl)
        {
            memset(newUrl, 0, newUrlLength);
            sprintf(newUrl, "%s/", self->BaseUrl);
            bSuccess = CreatorHTTP_EncodePathSegmentRFC3986(suffix, newUrl + urlLength + 1, newUrlLength - urlLength - 1, NULL);
            Creator_MemFree((void **)&self->BaseUrl);
            self->BaseUrl = newUrl;
        }
        else
        {
            bSuccess = false;
        }
    }

    if (!bSuccess)
    {
        //failure should discard base url to screw up the query and indicate the failure
        Creator_MemFree((void **)&self->BaseUrl);
    }
    return bSuccess;
}

bool CreatorHTTPQuery_AppendPathSuffixBoolean(CreatorHTTPQuery query, bool value)
{
    return CreatorHTTPQuery_AppendPathSuffix(query, value ? "true" : "false");
}

bool CreatorHTTPQuery_AppendPathSuffixChar(CreatorHTTPQuery query, char value)
{
    char szChar[2] = "\0";
    szChar[0] = value;
    return CreatorHTTPQuery_AppendPathSuffix(query, szChar);
}

bool CreatorHTTPQuery_AppendPathSuffixInt(CreatorHTTPQuery query, int value)
{
    char szInt[32];
    sprintf(szInt, "%d", value);
    return CreatorHTTPQuery_AppendPathSuffix(query, szInt);
}

static size_t CreatorHTTPQuery_FindParameterLength(char* url)
{
    size_t result = 0;
    char* position = url;
    if (position)
    {
        while ((*position != '&') && (*position != '\0'))
        {
            result++;
            position++;
        }
    }
    return result;
}

void CreatorHTTPQuery_Free(CreatorHTTPQuery *self)
{
    if (self && *self)
    {
        CreatorHTTPQuery httpQuery = *self;
        uint index;
        for (index = 0; index < httpQuery->Used; index++)
        {
            if (httpQuery->Parameters[index])
            {
                if (httpQuery->Parameters[index]->Key)
                    Creator_MemFree((void **)&httpQuery->Parameters[index]->Key);
                if (httpQuery->Parameters[index]->Value)
                    Creator_MemFree((void **)&httpQuery->Parameters[index]->Value);
                Creator_MemFree((void **)&httpQuery->Parameters[index]);
                httpQuery->Parameters[index] = NULL;
            }
        }
        Creator_MemFree((void **)&httpQuery->Parameters);
        if (httpQuery->BaseUrl)
        {
            Creator_MemFree((void **)&httpQuery->BaseUrl);
        }
        Creator_MemFree((void **)self);
    }
}

char* CreatorHTTPQuery_GenerateUrl(CreatorHTTPQuery self)
{
    char* result = NULL;
    if (self && self->BaseUrl)
    {
        size_t urlLength = strlen(self->BaseUrl);
        size_t totalSize = urlLength + 1/*\0*/;

        uint index;
        uint componentsCount = 0;

        const char *components[self->Used * 4];
        size_t componentsLength[self->Used * 4];

        for (index = 0; index < self->Used; index++)
        {
            if (!(self->Parameters[index]))
            {
                continue;
            }
            components[componentsCount] = self->Parameters[index]->Key;
            totalSize += (componentsLength[componentsCount++] = strlen(self->Parameters[index]->Key));
            components[componentsCount] = "=";
            totalSize += (componentsLength[componentsCount++] = 1);
            components[componentsCount] = self->Parameters[index]->Value;
            totalSize += (componentsLength[componentsCount++] = strlen(self->Parameters[index]->Value));
            components[componentsCount] = "&";
            totalSize += (componentsLength[componentsCount++] = 1);
        }

        result = (char*)Creator_MemAlloc(totalSize);
        memset(result, 0, totalSize);
        memcpy(result, self->BaseUrl, urlLength);
        size_t writtenBytes = urlLength;
        if (componentsCount > 0)
        {
            result[writtenBytes++] = '?';
            for (index = 0; index < (componentsCount - 1); index++)
            {
                if (componentsLength[index] == 1)
                {
                    result[writtenBytes++] = *components[index];
                }
                else
                {
                    memcpy(result + writtenBytes, components[index], componentsLength[index]);
                    writtenBytes += componentsLength[index];
                }
            }
        }

    }
    return result;
}

char *CreatorHTTPQuery_GetBaseUrl(CreatorHTTPQuery self)
{
    char *result = NULL;
    if (self)
    {
        result = oauth_url_unescape(self->BaseUrl, NULL);
    }
    return result;
}

char *CreatorHTTPQuery_GetQueryParameter(CreatorHTTPQuery self, const char *param)
{
    char *result = NULL;
    if (self)
    {
        uint index;
        for (index = 0; index < self->Used; index++)
        {
            if (strcmp(self->Parameters[index]->Key, param) == 0)
            {
                result = oauth_url_unescape(self->Parameters[index]->Value, NULL);
                break;
            }
        }
    }
    return result;
}

static void CreatorHTTPQuery_LoadParameters(CreatorHTTPQuery query, const char* url)
{
    if (url && (url[0] == '?') && url[1] != '\0')
    {
        char* part = (char*)&url[1];
        while (part)
        {
            size_t partLength = CreatorHTTPQuery_FindParameterLength(part);
            if (partLength == 0)
            {
                part = NULL;
            }
            else
            {
                char* equalPosition = strchr(part, '=');
                if (equalPosition)
                {
                    equalPosition++;
                    size_t valueLength = partLength - (equalPosition - part);
                    char* value = (char*)Creator_MemAlloc((valueLength + 1) * sizeof(char));
                    memcpy(value, equalPosition, valueLength + 1);
                    value[valueLength] = '\0';
                    size_t keyLength = partLength - valueLength - 1;
                    char* key = (char*)Creator_MemAlloc((keyLength + 1) * sizeof(char));
                    memcpy(key, part, keyLength);
                    key[keyLength] = '\0';
                    CreatorHTTPQuery_AddParameters(query, key, value);
                }
                part += partLength;
                if (*part == '&')
                    part++;
            }
        }
    }
}

CreatorHTTPQuery CreatorHTTPQuery_New()
{
    size_t size = sizeof(struct _CreatorHTTPQuery);
    CreatorHTTPQuery result = (CreatorHTTPQuery)Creator_MemAlloc(size);
    if (result)
    {
        memset(result, 0, size);
        size = CREATOR_HTTPQUERYPARAMETER_DEFAULTSIZE * sizeof(CreatorHTTPQueryParameter);
        result->Parameters = (CreatorHTTPQueryParameter *)Creator_MemAlloc(size);
        if (result->Parameters)
        {
            memset(result->Parameters, 0, size);
            result->Size = CREATOR_HTTPQUERYPARAMETER_DEFAULTSIZE;
        }
    }
    return result;
}

CreatorHTTPQuery CreatorHTTPQuery_NewFromUrl(const char* url)
{
    CreatorHTTPQuery result = CreatorHTTPQuery_New();
    if (result)
    {
        //find length of base url excluding query parameters
        char *urlBaseEnd = strchr(url, '?');
        size_t urlLength;
        if (urlBaseEnd)
        {
            urlLength = urlBaseEnd - url;
        }
        else
        {
            urlLength = strlen(url);
        }
        result->BaseUrl = Creator_MemAlloc(urlLength + 1);
        if (result->BaseUrl)
        {
            memcpy(result->BaseUrl, url, urlLength);
            result->BaseUrl[urlLength] = '\0';
            if (urlBaseEnd)
            {
                CreatorHTTPQuery_LoadParameters(result, urlBaseEnd);
            }
        }
        else
        {
            CreatorHTTPQuery_Free(&result);
        }

    }
    return result;
}

void CreatorHTTPQuery_SetBaseUrl(CreatorHTTPQuery self, const char *szBaseUrl)
{
    if (!self || !szBaseUrl)
    {
        return;
    }
    if (self->BaseUrl)
    {
        Creator_MemFree((void **)&self->BaseUrl);
    }
    size_t urlLength = strlen(szBaseUrl);
    self->BaseUrl = Creator_MemAlloc(urlLength + 1);
    if (self->BaseUrl)
    {
        strcpy(self->BaseUrl, szBaseUrl);
    }
}

void CreatorHTTPQuery_SetQueryParameter(CreatorHTTPQuery self, const char *param, const char* paramValue)
{
    if (self)
    {
        bool found = false;
        uint index;
        for (index = 0; index < self->Used; index++)
        {
            if (strcmp(self->Parameters[index]->Key, param) == 0)
            {
                if (self->Parameters[index]->Value)
                    Creator_MemFree((void **)&self->Parameters[index]->Value);
                if (paramValue)
                    self->Parameters[index]->Value = oauth_url_unescape(paramValue, NULL);
                else
                    self->Parameters[index]->Value = NULL;
                found = true;
                break;
            }
        }
        if (!found)
        {
            char *key = CreatorString_Duplicate((char *)param);
            CreatorHTTPQuery_AddParameters(self, key, oauth_url_escape(paramValue));
        }
    }
}

