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

/*! \file http_server.c
 *  \brief LibCreatorCore .
 */
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#ifdef MICROCHIP_PIC32
#include <stdint.h>
#include <tcpip/berkeley_api.h>
#include <sys/errno.h>
typedef int socklen_t;
#define htons(p) p
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
typedef int SOCKET;
#define SOCKET_ERROR            (-1)
#define closesocket close
#include <netinet/in.h>
#include <linux/tcp.h>
#include <linux/version.h>
#endif

#include "creator/core/http_server.h"
#include "creator/core/base_types.h"
#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_list.h"
#include "creator/core/creator_threading.h"
#include "creator/core/base_types_methods.h"
#include "creator/core/common_messaging_main.h"
#include "common_messaging_parser.h"
#include "creator_tls.h"

typedef struct CreatorHTTPServerImpl
{
    int AddressFamily;
    int Port;
    bool Secure;
    uint8 *Cert;
    int CertLength;
    int CertType;
    SOCKET ServerSocket;
    CreatorHTTPServer_ProcessRequest RequestCallback;
} HTTPServer;

static CreatorSemaphore _ServersLock = NULL;
static CreatorList _Servers;
static CreatorThread _ListenThread;
static bool _Terminate;

typedef struct CreatorHTTPServerRequestImpl
{
    CreatorHTTPServer Server;
    CreatorCommonMessaging_ControlBlock *ControlBlock;
    CreatorHTTPMethod Method;
    CreatorHTTPQuery Url;
    char *ContentType;
    void *Content;
    int ContentLength;
    int CurrentContentPosition;
    bool SentResponse;
} HTTPServerRequest;

static void ListenForClient(CreatorThread thread, void *context);
static bool HttpProtocolCallBack(CreatorCommonMessaging_CallbackEventType callbackEvent, char *headerName, char *value, int length, void *context);
void ParseMethodUrl(HTTPServerRequest *request, char *line, int length);


void CreatorHTTPServerRequest_Free(CreatorHTTPServerRequest *self)
{
    if (self && *self)
    {
        CreatorHTTPServerRequest request = *self;
        if (request->Url)
            CreatorHTTPQuery_Free(&request->Url);
        if (request->Content)
            Creator_MemFree(&request->Content);
        if (request->ContentType)
            CreatorString_Free(&request->ContentType);
        Creator_MemFree((void **)self);
    }
}

void *CreatorHTTPServerRequest_GetContent(CreatorHTTPServerRequest self)
{
    void *result = NULL;
    if (self)
    {
        result = self->Content;
    }
    return result;
}

int CreatorHTTPServerRequest_GetContentLength(CreatorHTTPServerRequest self)
{
    int result = 0;
    if (self)
    {
        result = self->ContentLength;
    }
    return result;
}

CreatorHTTPMethod CreatorHTTPServerRequest_GetMethod(CreatorHTTPServerRequest self)
{
    CreatorHTTPMethod result = CreatorHTTPMethod_NotSet;
    if (self)
    {
        result = self->Method;
    }
    return result;
}

CreatorHTTPQuery CreatorHTTPServerRequest_GetUrl(CreatorHTTPServerRequest self)
{
    CreatorHTTPQuery result = NULL;
    if (self)
    {
        result = self->Url;
    }
    return result;
}

char *AddCrLf(char *text)
{
    *text = '\r';
    text++;
    *text = '\n';
    text++;
    return text;
}

void CreatorHTTPServerRequest_SendResponse(CreatorHTTPServerRequest self, int statusCode, const char *contenType, void * content, int contentLength, bool closeConnection)
{
    uint8 headers[1024];
    memset(headers, 0, 1024);
    char *header = (char *)headers;
    memcpy(header, "HTTP/1.1 ", 9);
    header += 9;
    int length = snprintf(header, sizeof(32), "%d", statusCode);
    header += length;
    header = AddCrLf(header);

    if (closeConnection)
    {
        memcpy(header, "Connection: close", 17);
        header += 17;
        header = AddCrLf(header);
    }

    if (contenType)
    {
        length = strlen(contenType);
        memcpy(header, "Content-Type: ", 14);
        header += 14;
        memcpy(header, contenType, length);
        header += length;
        header = AddCrLf(header);
    }

    memcpy(header, "Content-Length: ", 16);
    header += 16;
    length = snprintf(header, sizeof(32), "%d", contentLength);
    header += length;
    header = AddCrLf(header);
    header = AddCrLf(header);
    CreatorCommonMessaging_SendRequest((void *)self->ControlBlock, (void *)headers, header - (char *)headers, 0);
    if (contentLength > 0)
        CreatorCommonMessaging_SendRequest((void *)self->ControlBlock, content, contentLength, 0);
    self->SentResponse = true;
}

void CreatorHTTPServer_Initialise(void)
{
    _ServersLock = CreatorSemaphore_New(1, 0);
    _Servers = CreatorList_New(5);
    _ListenThread = CreatorThread_New("Http", 1, 4096, ListenForClient, NULL);
}

CreatorHTTPServer CreatorHTTPServer_New(int port, bool secure, CreatorHTTPServer_ProcessRequest requestCallback)
{
    CreatorHTTPServer result;
    size_t size = sizeof(struct CreatorHTTPServerImpl);
    result = Creator_MemAlloc(size);
    if (result)
    {
        memset(result, 0, size);
        result->AddressFamily = AF_INET;
        result->Port = port;
        result->Secure = secure;
        result->RequestCallback = requestCallback;
    }
    return result;
}

void CreatorHTTPServer_SetCertificate(CreatorHTTPServer self, uint8 *cert, int certLength, int certType)
{
    if (self)
    {
        self->Cert = cert;
        self->CertLength = certLength;
        self->CertType = certType;
    }
}

void CreatorHTTPServer_Shutdown(void)
{
    _Terminate = true;
    if (_ListenThread)
    {
        CreatorThread_Join(_ListenThread);
        CreatorThread_Free(&_ListenThread);
    }
    if (_Servers)
        CreatorList_Free(&_Servers, false);
    if (_ServersLock)
        CreatorSemaphore_Free(&_ServersLock);
}

bool CreatorHTTPServer_Start(CreatorHTTPServer self)
{
    bool result = false;
    if (self)
    {
        SOCKET serverSocket = SOCKET_ERROR;
        struct sockaddr *address = NULL;
        socklen_t addressLength = 0;
        if (self->AddressFamily == AF_INET)
        {
            serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (serverSocket != SOCKET_ERROR)
            {
                addressLength = sizeof(struct sockaddr_in);
                struct sockaddr_in *ipAddress = Creator_MemAlloc(addressLength);
                if (ipAddress)
                {
                    memset(ipAddress, 0, addressLength);
                    ipAddress->sin_family = AF_INET;
#ifdef MICROCHIP_PIC32
                    ipAddress->sin_addr.S_un.S_addr = INADDR_ANY;
#else
                    ipAddress->sin_addr.s_addr = INADDR_ANY;
#endif
                    ipAddress->sin_port = htons(self->Port);
                    address = (struct sockaddr *)ipAddress;
                }
            }
        }
        else if (self->AddressFamily == AF_INET6)
        {
            serverSocket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
            if (serverSocket != SOCKET_ERROR)
            {
                addressLength = sizeof(struct sockaddr_in6);
                struct sockaddr_in6 *ipAddress = Creator_MemAlloc(addressLength);
                if (ipAddress)
                {
                    memset(ipAddress, 0, addressLength);
                    ipAddress->sin6_family = AF_INET6;
                    //memset(&ipAddress->sin6_addr, 0, sizeof(ipAddress->sin6_addr));
                    //ipAddress->sin6_addr.__in6_u = IN6ADDR_ANY_INIT; //IN6ADDR_ANY_INIT
                    ipAddress->sin6_port = htons(self->Port);
                    address = (struct sockaddr *)ipAddress;
                }
            }
        }

        if (address && (serverSocket != SOCKET_ERROR))
        {
#ifndef MICROCHIP_PIC32
            int flag = fcntl(serverSocket, F_GETFL);
            flag = flag | O_NONBLOCK;
            if (fcntl(serverSocket, F_SETFL, flag) < 0)
            {

            }
#endif
            if (bind(serverSocket, address, addressLength) != SOCKET_ERROR)
            {
                if (listen(serverSocket, 2) != SOCKET_ERROR)
                {
                    self->ServerSocket = serverSocket;
                    CreatorSemaphore_Wait(_ServersLock, 1);
                    CreatorList_Add(_Servers, self);
                    CreatorSemaphore_Release(_ServersLock, 1);
                    result = true;
                }
            }
            Creator_MemFree((void **)&address);
        }
        if (!result)
        {
            if (serverSocket != SOCKET_ERROR)
                closesocket(serverSocket);
        }

    }
    return result;
}

void CreatorHTTPServer_Stop(CreatorHTTPServer self)
{
    if (self)
    {
        CreatorSemaphore_Wait(_ServersLock, 1);
        CreatorList_Remove(_Servers, self);
        CreatorSemaphore_Release(_ServersLock, 1);
        closesocket(self->ServerSocket);
    }
}

void CreatorHTTPServer_Free(CreatorHTTPServer *self)
{
    if (self && *self)
    {
        Creator_MemFree((void **)self);
    }
}

static bool HttpProtocolCallBack(CreatorCommonMessaging_CallbackEventType callbackEvent, char *headerName, char *value, int length, void *context)
{
    bool result = true;
    HTTPServerRequest *request = (HTTPServerRequest *)context;
    switch (callbackEvent) {
        case CreatorCommonMessaging_CallbackEventType_Response:
            ParseMethodUrl(request, value, length);
            break;
        case CreatorCommonMessaging_CallbackEventType_Header:
            if (strcmp(headerName, "Content-Length") == 0)
            {
                request->ContentLength = (uint32)strtol(value, NULL, 10);
                if (request->ContentLength != 0)
                {
                    request->CurrentContentPosition = 0;
                    request->Content = Creator_MemAlloc(request->ContentLength);
                }
            }
            else if (strcasecmp(headerName, "Content-Type") == 0)
            {
                if (request->ContentType)
                    CreatorString_Free(&request->ContentType);
                request->ContentType = CreatorString_DuplicateWithLength(value, length);
            }
            break;
        case CreatorCommonMessaging_CallbackEventType_HeaderEnd:
            break;
        case CreatorCommonMessaging_CallbackEventType_Data:
            if ((request->ContentLength != 0) && (request->Content != NULL))
            {
                memcpy((request->Content + request->CurrentContentPosition), value, length);
                request->CurrentContentPosition += length;
            }
            break;
        case CreatorCommonMessaging_CallbackEventType_Finished:
            if (request->Server->RequestCallback)
            {
                request->Server->RequestCallback(request->Server, request);
            }
            if (!request->SentResponse)
                CreatorHTTPServerRequest_SendResponse(request, CreatorHTTPStatus_NotFound, NULL, NULL, 0, true);
            break;
        case CreatorCommonMessaging_CallbackEventType_NetworkFailure:
            break;
    }
    return result;
}

static void ListenForClient(CreatorThread thread, void *context)
{
    CreatorList clients = CreatorList_New(5);
    while (!_Terminate)
    {
        int index;
        CreatorSemaphore_Wait(_ServersLock, 1);
        for (index = 0; index < CreatorList_GetCount(_Servers); index++)
        {
            CreatorHTTPServer server = (CreatorHTTPServer)CreatorList_GetItem(_Servers, index);
            if (server)
            {
                struct sockaddr address =
                { 0 };
                socklen_t addressLength = sizeof(struct sockaddr);
                SOCKET client = accept(server->ServerSocket, &address, &addressLength);
                if (client != SOCKET_ERROR)
                {
#ifndef MICROCHIP_PIC32
                    int flag = fcntl(client, F_GETFL);
                    flag = flag | O_NONBLOCK;
                    if (fcntl(client, F_SETFL, flag) < 0)
                    {

                    }
#endif
                    size_t size = sizeof(CreatorCommonMessaging_ControlBlock);
                    CreatorCommonMessaging_ControlBlock *clientControl = Creator_MemAlloc(size);
                    if (clientControl)
                    {
                        memset(clientControl, 0, size);
                        size = sizeof(HTTPServerRequest);
                        HTTPServerRequest *request = Creator_MemAlloc(size);
                        if (request)
                        {
                            memset(request, 0, size);
                            request->Server = server;
                            request->ControlBlock = clientControl;
                            clientControl->ConnectionHandle = client;
                            clientControl->Enabled = true;
                            clientControl->ProtocolCallBack = HttpProtocolCallBack;
                            clientControl->CallbackContext = request;
                            clientControl->ReceivedBuffer = Creator_MemAlloc(CREATOR_MAX_PACKET_LEN);
                            if (clientControl->ReceivedBuffer)
                                memset(clientControl->ReceivedBuffer, 0, CREATOR_MAX_PACKET_LEN);

                            clientControl->IsPacketBegining = true;
                            if (server->Secure)
                            {
                                clientControl->TransportType = CREATOR_TLS;
                                clientControl->TLSCertificateData = server->Cert;
                                clientControl->TLSCertificateSize = server->CertLength;
                                if (!CreatorTLS_StartSSLSession(clientControl, false))
                                {
                                    if (clientControl->ReceivedBuffer)
                                        Creator_MemFree((void **)&clientControl->ReceivedBuffer);
                                    Creator_MemFree((void **)&clientControl);
                                    CreatorHTTPServerRequest_Free(&request);
                                    closesocket(client);
                                }
                                else
                                    CreatorList_Add(clients, (void *)clientControl);
                            }
                            else
                                CreatorList_Add(clients, (void *)clientControl);
                        }
                        else
                        {
                            Creator_MemFree((void **)&clientControl);
                            closesocket(client);
                        }
                    }
                }
            }
        }
        CreatorSemaphore_Release(_ServersLock, 1);

        index = 0;
        while (index < CreatorList_GetCount(clients))
        {
            CreatorCommonMessaging_ControlBlock *clientControl = (CreatorCommonMessaging_ControlBlock *)CreatorList_GetItem(clients, index);
            if (clientControl)
            {
                CreatorCommonMessaging_Receive(clientControl);
                if (!clientControl->Enabled || clientControl->ConnectionHandle == SOCKET_ERROR)
                {
                    CreatorHTTPServerRequest_Free((CreatorHTTPServerRequest *)&clientControl->CallbackContext);
                    if (clientControl->ReceivedBuffer)
                        Creator_MemFree((void **)&clientControl->ReceivedBuffer);
                    if (clientControl->SSLSession)
                        CreatorTLS_EndSSLSession(clientControl);
                    if (clientControl->ConnectionHandle != SOCKET_ERROR)
                        closesocket(clientControl->ConnectionHandle);
                    Creator_MemFree((void **)&clientControl);
                    CreatorList_RemoveAt(clients, index);
                }
                else
                    index++;
            }
            else
                index++;
        }
        CreatorThread_SleepMilliseconds(thread, 1);
    }
    int index = 0;
    while (index < CreatorList_GetCount(clients))
    {
        CreatorCommonMessaging_ControlBlock *clientControl = (CreatorCommonMessaging_ControlBlock *)CreatorList_GetItem(clients, index);
        if (clientControl)
        {
            CreatorHTTPServerRequest_Free((CreatorHTTPServerRequest *)&clientControl->CallbackContext);
            if (clientControl->ReceivedBuffer)
                Creator_MemFree((void **)&clientControl->ReceivedBuffer);
            if (clientControl->SSLSession)
                CreatorTLS_EndSSLSession(clientControl);
            if (clientControl->ConnectionHandle != SOCKET_ERROR)
                closesocket(clientControl->ConnectionHandle);
            Creator_MemFree((void **)&clientControl);
            CreatorList_RemoveAt(clients, index);
        }
        else
            index++;
    }
    CreatorList_Free(&clients, false);
}

void ParseMethodUrl(HTTPServerRequest *request, char *line, int length)
{
    if (length > 6)
    {
        int move = 0;
        if (strncmp(line, "GET ", 4) == 0)
        {
            request->Method = CreatorHTTPMethod_Get;
            move = 4;
        }
        else if (strncmp(line, "POST ", 5) == 0)
        {
            request->Method = CreatorHTTPMethod_Post;
            move = 5;
        }
        else if (strncmp(line, "PUT ", 4) == 0)
        {
            request->Method = CreatorHTTPMethod_Put;
            move = 4;
        }
        else if (strncmp(line, "DELETE ", 6) == 0)
        {
            request->Method = CreatorHTTPMethod_Delete;
            move = 6;
        }
        if (request->Method != CreatorHTTPMethod_NotSet)
        {
            length = length - move;
            line += move;
            //Find  HTTP/[Version] eg HTTP/1.0
            int index;
            bool found = false;
            for (index = length - 1; index > 0; index--)
            {
                if (line[index] == 'H')
                {
                    found = true;
                    length = index - 1;
                    break;
                }
            }
            if (found)
            {
                line[length] = '\0';
                if (request->Url)
                    CreatorHTTPQuery_Free(&request->Url);
                request->Url = CreatorHTTPQuery_NewFromUrl(line);
            }
        }
    }
}

