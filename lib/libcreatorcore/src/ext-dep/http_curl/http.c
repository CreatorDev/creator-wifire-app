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

#ifdef USE_CURL

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "creator/core/creator_debug.h"
#include "creator_http.h"
#include "creator/core/creator_memalloc.h"

//for backward-compatiblity
#ifdef CREATOR_DEBUG_ON
#ifndef CREATOR_HTTP_BYPASS_SSL_CHECK
#define CREATOR_HTTP_BYPASS_SSL_CHECK
#endif
#endif

typedef struct
{
    CURL *Curl;
    struct curl_slist *HeaderList;

    CreatorHTTPMethod Method;
    void *Body;
    size_t BodySize;
    size_t BodyWritten;
    bool BodyHandled;

    int HTTPResult;

    CreatorHTTPRequest_ResultCallback ResultCallback;
    CreatorHTTPRequest_HeaderCallback HeaderCallback;
    CreatorHTTPRequest_DataCallback DataCallback;
    CreatorHTTPRequest_FinishCallback FinishCallback;
    void *CallbackContext;
}RequestContext;

static size_t curlResponseDataReadFunction(char *ptr, size_t size, size_t nmemb, void *userdata);
static size_t curlRequestDataWriteFunction(void *ptr, size_t size, size_t nmemb, void *userdata);
static size_t curlResponseHeaderFunction(void *ptr, size_t size, size_t nmemb, void *userdata);

void CreatorHTTP_Initialise(void)
{
}

void CreatorHTTP_Shutdown(void)
{
}

CreatorHTTPRequest CreatorHTTPRequest_New(CreatorHTTPMethod method, const char *url, CreatorHTTPRequest_ResultCallback resultCallback, CreatorHTTPRequest_HeaderCallback headerCallback, CreatorHTTPRequest_DataCallback dataCallback, CreatorHTTPRequest_FinishCallback finishCallback, void *callbackContext)
{
    RequestContext *result = Creator_MemAlloc(sizeof(RequestContext));
    if (result)
    {
        CURL *curl = NULL;
        curl = result->Curl = curl_easy_init();
        if (curl)
        {
            result->HeaderList = NULL;
            result->HTTPResult = 0;

            result->Method = method;
            result->Body = NULL;
            result->BodySize = 0;
            result->BodyWritten = 0;
            result->BodyHandled = false;

            result->CallbackContext = callbackContext;
            result->ResultCallback = resultCallback;
            result->HeaderCallback = headerCallback;
            result->DataCallback = dataCallback;
            result->FinishCallback = finishCallback;

            curl_easy_setopt(curl, CURLOPT_URL, url);

            switch (method)
            {
                case CreatorHTTPMethod_Get:
                curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
                break;
                case CreatorHTTPMethod_Post:
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                break;
                case CreatorHTTPMethod_Put:
                curl_easy_setopt(curl, CURLOPT_PUT, 1L);
                curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
                break;
                case CreatorHTTPMethod_Delete:
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
                break;
                case CreatorHTTPMethod_Head:
                //			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "HEAD");
                break;
                default:
                break;
            }

            /* handle HTTP response body? */
            //	if (method == CreatorHTTPMethod_Head) {
            //		curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
            //	}
            //	else {
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlResponseDataReadFunction);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, result);
            //	}

            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curlResponseHeaderFunction);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, result);

#ifdef CREATOR_HTTP_BYPASS_SSL_CHECK
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
#endif

            CreatorHTTPRequest_AddHeader((CreatorHTTPRequest)result, "User-Agent", "libcurl; Creator REST Client");

#if LIBCURL_VERSION_NUM >= 0x071506
            curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
#else
            curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");
#endif
        }
        else
        {
            Creator_MemFree((void **)&result);
        }
    }
    return result;
}

void CreatorHTTPRequest_AddHeader(CreatorHTTPRequest self, const char *headerName, const char *headerValue)
{
    RequestContext *context = (RequestContext*)self;
    if (context)
    {
        char header[1024];
        if (strlen(headerName) + strlen(headerValue) + 3 <= sizeof(header))
        {
            strcpy(header, headerName);
            strcat(header, ": ");
            strcat(header, headerValue);
            context->HeaderList = curl_slist_append(context->HeaderList, header);
        }
    }
    /* FIXME assert or raise error somehow */
}

void CreatorHTTPRequest_SetBody(CreatorHTTPRequest self, const void *body, size_t bodySize)
{
    RequestContext *context = (RequestContext*)self;
    if (context)
    {
        context->BodyHandled = true;
        /* send HTTP request body */
        if (context->Method == CreatorHTTPMethod_Post)
        {
            curl_easy_setopt(context->Curl, CURLOPT_POSTFIELDSIZE, bodySize);
            curl_easy_setopt(context->Curl, CURLOPT_COPYPOSTFIELDS , body);
        }
        else if (context->Method == CreatorHTTPMethod_Put)
        {
            context->Body = Creator_MemAlloc(bodySize);
            if (context->Body)
            {
                memcpy(context->Body, body, bodySize);
                context->BodySize = bodySize;
                curl_easy_setopt(context->Curl, CURLOPT_INFILESIZE, (long)bodySize);
                curl_easy_setopt(context->Curl, CURLOPT_READFUNCTION, curlRequestDataWriteFunction);
                curl_easy_setopt(context->Curl, CURLOPT_READDATA, context);
            }
        }
    }
}

void CreatorHTTPRequest_Send(CreatorHTTPRequest self)
{
    RequestContext *context = (RequestContext*)self;
    CURLcode success;

    if (context->HeaderList)
    {
        curl_easy_setopt(context->Curl, CURLOPT_HTTPHEADER, context->HeaderList);
    }

    if (!context->BodyHandled && context->Method == CreatorHTTPMethod_Post)
    {
        curl_easy_setopt(context->Curl, CURLOPT_POSTFIELDSIZE, 0);
        curl_easy_setopt(context->Curl, CURLOPT_COPYPOSTFIELDS , "");
    }

    success = curl_easy_perform(context->Curl);
    if(success != CURLE_OK)
    {
        Creator_Log(CreatorLogLevel_Error, "HTTP %p failed: curl error %d", self, success);
    }
    CreatorHTTPError error = CreatorHTTPError_None;
    if (success != CURLE_OK)
    error = CreatorHTTPError_Unspecified;
    context->FinishCallback(self, context->CallbackContext, error);
}

void CreatorHTTPRequest_Free(CreatorHTTPRequest *self)
{
    if (self && *self)
    {
        RequestContext *context = (RequestContext*)*self;

        curl_easy_cleanup(context->Curl);

        curl_slist_free_all(context->HeaderList);
        context->HeaderList = NULL;

        if (context->Body)
        {
            Creator_MemFree((void **)&context->Body);
            context->BodySize = 0;
        }

        Creator_MemFree((void **)self);
    }
}

static size_t curlResponseDataReadFunction(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    RequestContext *context = (RequestContext*)userdata;
    if (context->HTTPResult)
    {
        context->DataCallback(context, context->CallbackContext, ptr, nmemb * size);
    }
    return nmemb * size;
}

static size_t curlRequestDataWriteFunction(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    RequestContext *context = (RequestContext*)userdata;
    size_t unwrittenSize = context->BodySize - context->BodyWritten, ptrSize = size * nmemb;
    size_t chunkSize = (unwrittenSize < ptrSize)?unwrittenSize:ptrSize;
    memcpy(ptr, ((char*)context->Body) + context->BodyWritten, chunkSize);
    context->BodyWritten += chunkSize;
    return chunkSize;
}

static size_t curlResponseHeaderFunction(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    RequestContext *context = (RequestContext*)userdata;
    size_t headerLength = nmemb * size;

    /* parse header: if HTTP XXX, call resultCallback. otherwise call headerCallback */
    if (headerLength >= sizeof("HTTP/1.X XXX")-1)
    {
        if (strncmp("HTTP/1", (char*)ptr, 5) == 0)
        {
            int result = (int)strtol((char*)ptr + 9, NULL, 10);
            //don't forward 100 Continue
            if (result > 0 && result != 100)
            {
                context->HTTPResult = result;
                context->ResultCallback(context, context->CallbackContext, (unsigned short)result);
                /* FIXME this is not robust... */
                return headerLength;
            }
        }
    }

    //send headers when final response is finally there
    if (context->HTTPResult && context->HeaderCallback)
    {
        const char *headerLine = (char*)ptr;
        const char *headerName = headerLine, *headerValue = NULL;
        size_t index, headerNameLength =0, headerValueLength=0;
        for (index = 0; index < headerLength; index++)
        {
            if (headerLine[index] == ':')
            {
                headerValue = headerLine + index + 1;
                headerNameLength = index;
                headerValueLength = headerLength - index - 1;
                /* strip CRLF from end of value */
                if (headerValueLength > 2 && headerValue[headerValueLength-2] == '\r' && headerValue[headerValueLength-1] == '\n')
                {
                    headerValueLength -= 2;
                }
                /* strip additional spaces */
                for (++index; index < headerLength; index++)
                {
                    if (headerLine[index] == ' ')
                    {
                        headerValue++;
                        headerValueLength--;
                    }
                    else
                    {
                        break;
                    }
                }
                break;
            }
        }
        if (headerName && headerValue && context->HeaderCallback)
        {
            context->HeaderCallback(context, context->CallbackContext, headerName, headerNameLength, headerValue, headerValueLength);
        }
    }

    return headerLength;
}

#endif
