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

/*! \file http_server.h
 *  \brief LibCreatorCore .
 */


#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#include <stdbool.h>
#include "creator/core/http_query.h"
#include "creator/core/creator_httpmethod.h"
#include "creator/core/creator_httpstatus.h"

typedef struct CreatorHTTPServerImpl *CreatorHTTPServer;
typedef struct CreatorHTTPServerRequestImpl *CreatorHTTPServerRequest;
typedef void (*CreatorHTTPServer_ProcessRequest)(CreatorHTTPServer server, CreatorHTTPServerRequest request);

void *CreatorHTTPServerRequest_GetContent(CreatorHTTPServerRequest self);
int CreatorHTTPServerRequest_GetContentLength(CreatorHTTPServerRequest self);
CreatorHTTPMethod CreatorHTTPServerRequest_GetMethod(CreatorHTTPServerRequest self);
CreatorHTTPQuery CreatorHTTPServerRequest_GetUrl(CreatorHTTPServerRequest self);
void CreatorHTTPServerRequest_SendResponse(CreatorHTTPServerRequest self, int statusCode, const char *contenType, void * content, int contentLength, bool closeConnection);


void CreatorHTTPServer_Initialise(void);

CreatorHTTPServer CreatorHTTPServer_New(int port, bool secure, CreatorHTTPServer_ProcessRequest requestCallback);

void CreatorHTTPServer_SetCertificate(CreatorHTTPServer self, uint8 *cert, int certLength, int certType);

void CreatorHTTPServer_Shutdown(void);

bool CreatorHTTPServer_Start(CreatorHTTPServer self);

void CreatorHTTPServer_Stop(CreatorHTTPServer self);

void CreatorHTTPServer_Free(CreatorHTTPServer *self);

#endif /* HTTP_SERVER_H_ */
