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

#ifndef USE_CURL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>


#include "creator_http.h"
#include "creator/core/creator_debug.h"
#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_threading.h"	
#include "creator/core/creator_cert.h"
#include "creator/core/errortype.h"
#include "creator/core/common_messaging_defines.h"
#include "creator/core/common_messaging_main.h"
#include "creator/core/base_types_methods.h"
//#include "creator/core/http_private.h"
#include "creator/core/creator_time.h"
#include "creator/core/creator_task_scheduler.h"

#ifdef MICROCHIP_PIC32
#ifdef CREATOR_HTTP_DEBUG
    #include "tcpip/src/system/system_debug.h"
#endif
#endif

struct HTTPRequestImpl;		// forward declaration

typedef struct
{
    void *Connection;
    char *HostName;		// TODO - allocate buffer for max hostname or malloc?
    uint32 HostAddress;
    CreatorCommonMessaging_ConnectionInformation ConnectionInfo;
    struct HTTPRequestImpl *Request;
    time_t LastUsed;
    CreatorTaskID CloseConnectionTaskID;
    CreatorSemaphore RequestMutex;
} HTTPClient;

typedef struct HTTPRequestImpl
{
    bool InUse;
    HTTPClient *HTTPClient;
    // TODO - add error to be sent in finished c/b
    CreatorHTTPMethod Method;
    char *Buffer;
    size_t BodyLength;
    size_t BufferLength;
    size_t BufferSize;
    int HTTPResult;

    CreatorHTTPRequest_ResultCallback ResultCallback;
    CreatorHTTPRequest_HeaderCallback HeaderCallback;
    CreatorHTTPRequest_DataCallback DataCallback;
    CreatorHTTPRequest_FinishCallback FinishCallback;
    void *CallbackContext;
} HTTPRequest;

#ifndef MAX_HTTP_CONNECTIONS
#define MAX_HTTP_CONNECTIONS	2
#endif

#define MAX_HEADER_SIZE			1024
#define MAX_HTTP_MESSAGE_SIZE	(40*1024)	// 40KB : TODO - define project/platform specific limits?
#define DEFAULT_MESSAGE_SIZE	(2*1024)	// 2KB
#define INCREASE_MESSAGE_SIZE	(DEFAULT_MESSAGE_SIZE)

#define HTTP_RESPONSE_TIME		60			// Response timeout (seconds)

#define INACTIVITY_TIMEOUT		10

static HTTPClient _HTTPClient[MAX_HTTP_CONNECTIONS];
static HTTPRequest _HTTPRequest[MAX_HTTP_CONNECTIONS];
bool _HTTPInitialised = false;

static void CheckRequestBuffer(HTTPRequest *request, size_t size);
static void CloseConnection(HTTPClient *client);
static void CloseConnectionTask(CreatorTaskID taskID, void *context);
static bool CreatorHTTPCallback(CreatorCommonMessaging_CallbackEventType event, char *headerName, char *value, int len, void *context);
static void CreatorHTTPResponseFinishedCallback(HTTPClient *client, HTTPRequest *request, CreatorHTTPError error);
static HTTPClient *GetClient(const char *url, const char **requestUri);


void CreatorHTTP_Initialise(void)
{
    int index;

    memset(_HTTPClient, 0, sizeof(_HTTPClient));
    memset(_HTTPRequest, 0, sizeof(_HTTPRequest));
    for (index = 0; index < MAX_HTTP_CONNECTIONS; index++)
    {
        HTTPClient *client = &_HTTPClient[index];
        client->RequestMutex = CreatorSemaphore_New(1, 0);
        // Limit requests to 1 per client connection (for now)
        client->Request = &_HTTPRequest[index];
        client->CloseConnectionTaskID = CreatorScheduler_ScheduleTask(CloseConnectionTask, (void *)client, INACTIVITY_TIMEOUT, true);
    }
    _HTTPInitialised = true;
}

void CreatorHTTP_Shutdown(void)
{
    int index;
    for (index = 0; index < MAX_HTTP_CONNECTIONS; index++)
    {
        HTTPClient *client = &_HTTPClient[index];
        if (client->HostName)
            Creator_MemFree((void **)&client->HostName);
        if (client->RequestMutex)
            CreatorSemaphore_Free(&client->RequestMutex);
    }
    _HTTPInitialised = false;
}

CreatorHTTPRequest CreatorHTTPRequest_New(CreatorHTTPMethod method, const char *url, CreatorHTTPRequest_ResultCallback resultCallback,
        CreatorHTTPRequest_HeaderCallback headerCallback, CreatorHTTPRequest_DataCallback dataCallback, CreatorHTTPRequest_FinishCallback finishCallback,
        void *callbackContext)
{
    HTTPRequest *result = NULL;
    HTTPRequest *request = NULL;
    HTTPClient *client = NULL;
    const char *requestUri = NULL;

    if (!_HTTPInitialised)
    {
        // TODO - call init before App tasks start (for thread safety)
        CreatorHTTP_Initialise();
    }

    // Get client connection based on the protocol + server name
#ifdef CREATOR_HTTP_DEBUG
    SYS_CONSOLE_PRINT("HTTP: %s %s\r\n", CreatorHTTPMethod_ToString(method), url);
#endif
    client = GetClient(url, &requestUri);
    if (client)
    {
        // Note: only single request per client support for now (requests block on client mutex)
        request = client->Request;
        request->HTTPClient = client;
        request->HTTPResult = 0;
        request->Method = method;
        request->Buffer = Creator_MemAlloc(DEFAULT_MESSAGE_SIZE);
        if (request->Buffer)
        {
            request->BufferSize = DEFAULT_MESSAGE_SIZE;
            request->BufferLength = 0;
            request->BodyLength = 0;
            request->CallbackContext = callbackContext;
            request->ResultCallback = resultCallback;
            request->HeaderCallback = headerCallback;
            request->DataCallback = dataCallback;
            request->FinishCallback = finishCallback;
            request->InUse = true;

            // Write request line with default header
            request->BufferLength = sprintf(request->Buffer, "%s %s HTTP/1.1\r\n", CreatorHTTPMethod_ToString(method), requestUri);

            // TODO - define agent name
            CreatorHTTPRequest_AddHeader((CreatorHTTPRequest)request, "User-Agent", "c http client");
            CreatorHTTPRequest_AddHeader((CreatorHTTPRequest)request, "Host", client->HostName);

            // TODO - define accept-encoding? (Note: tcp stays alive for by itself - for awhile)
            //CreatorHTTPRequestAddHeader((CreatorHTTPRequest)request, "Accept-Encoding", "gzip, deflate");
            //CreatorHTTPRequestAddHeader((CreatorHTTPRequest)request, "Connection", "keep-alive");
            result = request;
        }
    }
    if (result == NULL)
    {
        Creator_Log(CreatorLogLevel_Error, "HTTP request failed...");	// TODO - Add reason...
        if (client)
        {
#ifdef CREATOR_HTTP_DEBUG_SEM
            SYS_CONSOLE_MESSAGE("\r\nCreatorSemaphore_ReleaseError2");
#endif
            CreatorSemaphore_Release(client->RequestMutex, 1);
        }
    }
    return result;
}

void CreatorHTTPRequest_AddHeader(CreatorHTTPRequest self, const char *headerName, const char *headerValue)
{
    HTTPRequest *request = (HTTPRequest*)self;
    int nameLength = strlen(headerName);
    int valueLength = strlen(headerValue);
    int length = nameLength + valueLength + 4;
    if (length <= MAX_HEADER_SIZE)
    {
        CheckRequestBuffer(request, length);
        if (request->Buffer)
        {
            char *buffer = request->Buffer + request->BufferLength;
            request->BufferLength += sprintf(buffer, "%s: %s\r\n", headerName, headerValue);
        }
    }
}

void CreatorHTTPRequest_SetBody(CreatorHTTPRequest self, const void *pBody, size_t bodySize)
{
    HTTPRequest *request = (HTTPRequest*)self;
    if (request->Method != CreatorHTTPMethod_Get && request->BodyLength == 0)
    {
        // TODO - fix check for length header room
        CheckRequestBuffer(request, bodySize + 24);
        if (request->Buffer)
        {
            char *buffer = request->Buffer + request->BufferLength;
#if i386
            request->BufferLength += sprintf(buffer, "Content-Length: %d\r\n\r\n", bodySize);
#else
            request->BufferLength += sprintf(buffer, "Content-Length: %ld\r\n\r\n", bodySize);
#endif
            buffer = &request->Buffer[request->BufferLength];
            memcpy(buffer, pBody, bodySize);
            request->BufferLength += bodySize;
            request->BodyLength = bodySize;
        }
    }
}

void CreatorHTTPRequest_Send(CreatorHTTPRequest self)
{
    HTTPRequest *request = (HTTPRequest*)self;
    HTTPClient *client = request->HTTPClient;
    CreatorHTTPError error = CreatorHTTPError_Unspecified;
    if (client)
    {
        CreatorScheduler_SetTaskInterval(client->CloseConnectionTaskID, HTTP_RESPONSE_TIME + INACTIVITY_TIMEOUT);
        if (request->BodyLength == 0)
        {
            // Terminate headers
            // TODO - fix check for length header room
            CheckRequestBuffer(request, 24);
            if (request->Buffer)
            {
                char *buffer = request->Buffer + request->BufferLength;
                if (request->Method == CreatorHTTPMethod_Get)
                {
                    memcpy(buffer, "\r\n", 2);
                    request->BufferLength += 2;
                }
                else
                {
                    request->BufferLength += sprintf(buffer, "Content-Length: 0\r\n\r\n");
                }
            }
        }

        if (request->Buffer)
        {
            if (!client->Connection)
            {
                if (client->ConnectionInfo.ConnectionDestinationAddress == 0)
                {
                    // Get host address (DNS lookup)
                    int attemptCount = 0;
                    while ((client->ConnectionInfo.ConnectionDestinationAddress == 0) && (attemptCount < 2))
                    {
                        client->ConnectionInfo.ConnectionDestinationAddress = CreatorCommonMessaging_GetHostByName(client->HostName);
                        attemptCount++;
                    }
                    if (client->ConnectionInfo.ConnectionDestinationAddress == 0)
                    {
                        client->ConnectionInfo.ConnectionDestinationAddress = client->HostAddress;
                        Creator_Log(CreatorLogLevel_Error, "HTTP send failed (DNS lookup): %s", CreatorHTTPMethod_ToString(request->Method));
                    }
                    else
                    {
                        client->HostAddress = client->ConnectionInfo.ConnectionDestinationAddress;
                    }
                }
                if (client->ConnectionInfo.ConnectionDestinationAddress != 0)
                {
                    // Open host connection
                    client->Connection = CreatorCommonMessaging_CreateConnection(&client->ConnectionInfo, CreatorHTTPCallback, client);
                    if (!client->Connection)
                        client->ConnectionInfo.ConnectionDestinationAddress = 0;
                }
            }

            if (client->Connection)
            {
                int result = CreatorCommonMessaging_SendRequest(client->Connection, request->Buffer, request->BufferLength, HTTP_RESPONSE_TIME);
                if (result == 0)
                {
                    CloseConnection(client);
                    Creator_Log(CreatorLogLevel_Error, "HTTP send failed: %s, total length=%d, content-length=%d", CreatorHTTPMethod_ToString(request->Method),
                            request->BufferLength, request->BodyLength);
                }
                else
                {
                    error = CreatorHTTPError_None;
                }
            }
        }
    }
    if (error != CreatorHTTPError_None)
    {
        // Failed
#ifdef CREATOR_HTTP_DEBUG
        SYS_CONSOLE_PRINT("\r\nHTTP Send failed - dport=%d\r\n", client->ConnectionInfo.ConnectionDestinationPort);
#endif
        CreatorHTTPResponseFinishedCallback(client, request, error);
    }
}

void CreatorHTTPRequest_Free(CreatorHTTPRequest *self)
{
    if (self && *self)
    {
        HTTPRequest *request = (HTTPRequest*)*self;
        if (request->Buffer)
        {
            Creator_MemFree((void **)&request->Buffer);
            request->BufferSize = 0;
        }
        if (request->InUse)
        {
#ifdef CREATOR_HTTP_DEBUG_SEM
            SYS_CONSOLE_MESSAGE("\r\nCreatorSemaphore_Release Finish");
#endif
#ifdef CREATOR_HTTP_DEBUG
            SYS_CONSOLE_MESSAGE("  HTTP: done\r\n");
#endif
            request->InUse = false;
        }
        CreatorSemaphore_Release(request->HTTPClient->RequestMutex, 1);
        self = NULL;
    }
}

static bool CreatorHTTPResponseResultCallback(char *ptr, size_t headerLength, HTTPRequest *request)
{
    bool result = false;
    // Parse the response line
    if (headerLength >= sizeof("HTTP/1.X XXX") - 1)
    {
        if (strncmp("HTTP/1", (char*)ptr, 5) == 0)
        {
            int statusCode = (int)strtol((char*)ptr + 9, NULL, 10);
            // Don't forward 100 Continue
            if (statusCode > 0 && statusCode != 100)
            {
                result = true;
                request->HTTPResult = statusCode;
                request->ResultCallback(request, request->CallbackContext, (unsigned short)statusCode);
                // FIXME not robust... ?
            }
        }
    }
    return result;
}

static void CreatorHTTPResponseHeaderCallback(char *headerName, char *value, size_t headerValueLength, HTTPRequest *request)
{
    // Send headers after response result has been received
    if (request->HTTPResult && request->HeaderCallback)
    {
        int headerNameLength = strlen(headerName);
        if (headerName && headerNameLength > 0 && value && request->HeaderCallback)
        {
            request->HeaderCallback(request, request->CallbackContext, headerName, headerNameLength, value, headerValueLength);
        }
    }
}

static void CreatorHTTPResponseDataCallback(char *responseData, size_t dataLength, HTTPRequest *request)
{
    if (request->HTTPResult && request->DataCallback)
    {
        request->DataCallback(request, request->CallbackContext, responseData, dataLength);
    }
}

static void CreatorHTTPResponseFinishedCallback(HTTPClient *client, HTTPRequest *request, CreatorHTTPError error)
{
    if (request->FinishCallback)
    {
        request->FinishCallback(request, request->CallbackContext, error);
    }
}

bool CreatorHTTPCallback(CreatorCommonMessaging_CallbackEventType callbackEvent, char *headerName, char *value, int length, void *context)
{
    bool result = true;
    HTTPClient *client = (HTTPClient *)context;
    HTTPRequest *request = NULL;
    if (client)
    {
        request = client->Request;
    }
    if (request && request->InUse)
    {
        switch (callbackEvent) {
            case CreatorCommonMessaging_CallbackEventType_Response:
            {
                result = CreatorHTTPResponseResultCallback(value, length, request);
                break;
            }
            case CreatorCommonMessaging_CallbackEventType_Header:
            {
                CreatorHTTPResponseHeaderCallback(headerName, value, length, request);
                break;
            }
            case CreatorCommonMessaging_CallbackEventType_Data:
            {
                if (value && length > 0)
                {
                    CreatorHTTPResponseDataCallback(value, length, request);
                }
                break;
            }
            case CreatorCommonMessaging_CallbackEventType_Finished:
            {
                CreatorScheduler_SetTaskInterval(client->CloseConnectionTaskID, INACTIVITY_TIMEOUT);
                CreatorHTTPResponseFinishedCallback(client, request, CreatorHTTPError_None);
                break;
            }
            case CreatorCommonMessaging_CallbackEventType_NetworkFailure:
            {
#ifdef CREATOR_HTTP_DEBUG
                SYS_CONSOLE_PRINT("\r\nHTTP Error - response timeout or connection lost: dport=%d\r\n", client->ConnectionInfo.ConnectionDestinationPort);
#endif
                CloseConnection(client);
                CreatorHTTPResponseFinishedCallback(client, request, CreatorHTTPError_NetworkFailure);
                break;
            }
            case CreatorCommonMessaging_CallbackEventType_HeaderEnd:
            {
                break;
            }
        }
    }
    else
    {
        if (client && callbackEvent == CreatorCommonMessaging_CallbackEventType_NetworkFailure)
        {
            CloseConnection(client);
#ifdef CREATOR_HTTP_DEBUG
            SYS_CONSOLE_PRINT("\r\nHTTP - connection lost: dport=%d (no Rx pending)\r\n", client->ConnectionInfo.ConnectionDestinationPort);
#endif
        }
    }
    return result;
}

static void CloseConnection(HTTPClient *client)
{
    if (client->Connection)
    {
        CreatorCommonMessaging_DeleteConnection(client->Connection);
        client->Connection = NULL;
    }
}

static void CloseConnectionTask(CreatorTaskID taskID, void *context)
{
    HTTPClient *client = (HTTPClient *)context;
    if (client)
    {
        CloseConnection(client);
    }
}

static HTTPClient *GetClient(const char *url, const char **requestUri)
{
    int index;
    HTTPClient *client = NULL;
    const char *hostName = NULL;
    bool isSSL = false;
    int port;
    int connectionTransportType;
    int hostNameLength = 0;
    // Use protocol to get the connection
    if (strncmp(url, "http://", 7) == 0)
    {
        port = 80;
        connectionTransportType = CREATOR_TCP;
        hostName = url + 7;
    }
    else //if (strncmp(url, "https://", 8) == 0)
    {
        port = 443;
        connectionTransportType = CREATOR_TLS;
        hostName = url + 8;
        isSSL = true;
    }

    if (*hostName)
    {
        // Get host name
        const char *uri = strchr(hostName, '/');
        if (uri)
        {
            hostNameLength = uri - hostName;
            *requestUri = uri;
        }
        else
        {
            hostNameLength = strlen(hostName);
            *requestUri = "/";
        }
    }

    time_t oldest;
    Creator_GetTime(&oldest);
    int bestIndex = -1;
    for (index = 0; index < MAX_HTTP_CONNECTIONS; index++)
    {
        if (_HTTPClient[index].HostName)
        {
            if (_HTTPClient[index].ConnectionInfo.ConnectionDestinationPort == port)
            {
                int length = strlen(_HTTPClient[index].HostName);
                if (length == hostNameLength && memcmp(_HTTPClient[index].HostName, hostName, hostNameLength) == 0)
                {
                    client = &_HTTPClient[index];
                    break;
                }
            }
            else
            {
                if (_HTTPClient[index].LastUsed < oldest)
                {
                    oldest = _HTTPClient[index].LastUsed;
                    bestIndex = index;
                }
            }
        }
        else
        {
            bestIndex = index;
            oldest = 0;
        }
    }

    if (client)
    {
        CreatorSemaphore_Wait(client->RequestMutex, 1);
        Creator_GetTime(&client->LastUsed);
    }
    else
    {
        client = &_HTTPClient[bestIndex];
#ifdef CREATOR_HTTP_DEBUG_SEM
        SYS_CONSOLE_MESSAGE("\r\nCreatorSemaphore_Wait");
#endif
        CreatorSemaphore_Wait(client->RequestMutex, 1);
        CloseConnection(client);
        client->ConnectionInfo.ConnectionDestinationPort = port;
        client->ConnectionInfo.ConnectionTransportType = connectionTransportType;
        client->ConnectionInfo.ConnectionDestinationAddress = 0;
        if (client->HostName)
            CreatorString_Free(&client->HostName);
        client->HostName = CreatorString_DuplicateWithLength(hostName, hostNameLength);
        client->HostAddress = 0;
        if (isSSL)
            client->ConnectionInfo.TLSCertificateData = CreatorCert_GetCertificate(client->HostName);
#ifdef CREATOR_HTTP_DEBUG_SEM
        SYS_CONSOLE_MESSAGE("\r\nGOT CreatorSemaphore_Wait");
#endif
        Creator_GetTime(&client->LastUsed);
    }

    return client;
}

static void CheckRequestBuffer(HTTPRequest *request, size_t size)
{
    // Reallocate if smaller buffer already allocated
    // Note - don't allocate buffer if previous reallocate already failed
    size_t newSize = request->BufferLength + size;
    if (request->Buffer && newSize > request->BufferSize)
    {
        bool success = false;
        if (newSize < MAX_HTTP_MESSAGE_SIZE)
        {
            // Round up new size to nearest re-allocation size
            newSize = ((newSize / INCREASE_MESSAGE_SIZE) + 1) * INCREASE_MESSAGE_SIZE;
            char *newBuffer = Creator_MemRealloc(request->Buffer, newSize);
            if (newBuffer)
            {
                request->Buffer = newBuffer;
                request->BufferSize = newSize;
                success = true;
            }
            else
            {
                Creator_Log(CreatorLogLevel_Error, "HTTP request failed - insufficient memory (buffer size = %d)", newSize);
            }
        }
        else
        {
            Creator_Log(CreatorLogLevel_Error, "HTTP request failed - message too big (buffer size = %d)", newSize);
        }

        if (!success)
        {
            // Message too big (DON'T send, just wait for release)
            // TODO - Set failed status or do error result callback on send request?
            Creator_MemFree((void **)&request->Buffer);
            request->BufferSize = 0;
        }
    }
}

#endif
