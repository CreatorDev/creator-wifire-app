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


#ifndef CREATOR_HTTP_H_
#define CREATOR_HTTP_H_

#include <stddef.h>
#include <stdbool.h>
#include "creator_httpmethod.h"

typedef enum
{
	CreatorHTTPError_NotSet = 0,
	CreatorHTTPError_None,
	CreatorHTTPError_Unspecified, 		//an error occured, but no-one knows what it was
	CreatorHTTPError_Timeout,
	CreatorHTTPError_NetworkFailure
} CreatorHTTPError;


typedef void *CreatorHTTPRequest;

/**
 * Called when (if) the HTTP response code was received.
 *
 * @param request http request instance.
 * @param context context passed to \ref CreatorHTTPRequest_New
 * @param httpResult HTTP response code
 */
typedef void (*CreatorHTTPRequest_ResultCallback)(CreatorHTTPRequest request, void *context, unsigned short httpResult);

/**
 * Called for each response header.
 *
 * @param request http request instance.
 * @param context context passed to \ref CreatorHTTPRequest_New
 * @param headerName name of the header
 * @param nameLength length of the header name (without \0)
 * @param headerValue value of the header
 * @param valueLength length of the header value (without \0)
 */
typedef void (*CreatorHTTPRequest_HeaderCallback)(CreatorHTTPRequest request, void *context, const char *headerName, size_t nameLength, const char *headerValue, size_t valueLength);

/**
 * Called when a new chunk of data is received from the server. May be called multiple times.
 *
 * @param request http request instance.
 * @param context context passed to \ref CreatorHTTPRequest_New
 * @param data chunk of data that was received
 * @param dataLength length in bytes of the chunk of data
 */
typedef void (*CreatorHTTPRequest_DataCallback)(CreatorHTTPRequest request, void *context, const char *data, size_t dataLength);

/**
 * Called when the request is finished (successfully or not).
 *
 * @param request http request instance.
 * @param context context passed to \ref CreatorHTTPRequest_New
 * @param error type of error that was encountered (CreatorHTTPError_None on success).
 */
typedef void (*CreatorHTTPRequest_FinishCallback)(CreatorHTTPRequest request, void *context, CreatorHTTPError error);

/**
 * \brief Add a request header to an existing HTTP handler.
 *
 * @param self HTTP handler returned by \ref CreatorHTTPRequest_New
 * @param headerName
 * @param headerValue
 */
void CreatorHTTPRequest_AddHeader(CreatorHTTPRequest self, const char *headerName, const char *headerValue);

/**
 * \brief Creates a new HTTP request handler.
 *
 * @param method method of the request
 * @param url url to request
 * @param resultCallback callback which handles the HTTP response code. may be NULL.
 * @param headerCallback callback which handles HTTP response headers. must be called for each handler. may be NULL.
 * @param dataCallback callback which handles response data. may be NULL.
 * @param finishCallback method to be called when the request is finished.
 * @param context pointer to be passed to callback methods.
 * @return the handler for this request, or NULL if this request cannot be created.
 */
CreatorHTTPRequest CreatorHTTPRequest_New(CreatorHTTPMethod method, const char *url, CreatorHTTPRequest_ResultCallback resultCallback, CreatorHTTPRequest_HeaderCallback headerCallback, CreatorHTTPRequest_DataCallback dataCallback, CreatorHTTPRequest_FinishCallback finishCallback, void *context);

/**
 * \brief Sends the request.
 *
 * @param self HTTP handler returned by \ref CreatorHTTPRequest_New
 */
void CreatorHTTPRequest_Send(CreatorHTTPRequest self);

/**
 * \brief Sets the body of a request.
 *
 * @param self HTTP handler returned by \ref CreatorHTTPRequest_New
 * @param body data to be sent
 * @param bodySize length of \a pBody
 */
void CreatorHTTPRequest_SetBody(CreatorHTTPRequest self, const void *body, size_t bodySize);


/**
 * \brief Frees/Releases the resources associated with the request handler.
 *
 * After this has been called, callbacks should not be called.
 *
 * @param self HTTP handler returned by \ref CreatorHTTPRequest_New
 */
void CreatorHTTPRequest_Free(CreatorHTTPRequest *self);



#ifdef CREATOR_HTTP_TEST
CreatorHTTPRequest CreatorHTTPRequest_New_PlatformDependent(CreatorHTTPMethod method, const char *url, CreatorHTTPRequest_ResultCallback resultCallback, CreatorHTTPRequest_HeaderCallback headerCallback, CreatorHTTPRequest_DataCallback dataCallback, CreatorHTTPRequest_FinishCallback finishCallback, void *context);

void CreatorHTTPRequestAddHeader_PlatformDependent(CreatorHTTPRequest self, const char *headerName, const char *headerValue);

void CreatorHTTPRequestSetBody_PlatformDependent(CreatorHTTPRequest self, const void *pBody, size_t bodySize);

void CreatorHTTPRequestSend_PlatformDependent(CreatorHTTPRequest self);

void CreatorHTTPRequestRelease_PlatformDependent(CreatorHTTPRequest self);
#endif

#endif /* CREATOR_HTTP_H_ */

