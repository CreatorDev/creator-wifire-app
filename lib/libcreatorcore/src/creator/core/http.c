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


#define _XOPEN_SOURCE /* enable strptime */
#define _GNU_SOURCE /* enable strptime */
#define _GNU_SOURCE /* for strnlen */
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <sys/types.h>

#include "oauth.h"

#include "creator/core/creator_debug.h"
#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_threading.h"
#include "creator_threading_private.h"
#include "creator_cache.h"

#include "creator/core/api.h"
#include "creator/core/base_types.h"
#include "creator/core/base_types_methods.h"
#include "creator/core/base_types_methods_private.h"
#include "creator/core/client_private.h"
#include "creator/core/creator_object.h"
#include "creator/core/creator_object_methods.h"
#include "creator/core/creator_object_private.h"
#include "creator/core/creator_memorymanager_methods.h"
#include "creator/core/http_private.h"
#include "creator/core/session_private.h"
#include "creator/core/server_private.h"
#include "creator/core/servertime.h"
#include "creator/core/badrequestresponse.h"
#include "creator/core/timeparse.h"
#include "creator/core/xml_serialisation.h"
#include "creator/core/xml_serialisation_private.h"

typedef struct HTTPCallbackContextImpl
{
	CreatorHTTPRequest Request;
	CreatorSemaphore Semaphore;
	bool Success;//false if can not setup http call or finish callback returns error, true otherwise
	CreatorXMLDeserialiser Deserialiser;
	CreatorMemoryManager MemoryManager;
	CreatorType ExpectedType;
	CreatorType ResponseType;//Used for error handling
	bool IsSuccessResponse;
	bool IsBadRequestResponse;
	bool HasParserError;
	CreatorDatetime Expires;
	CreatorHTTPStatus Status;
	CreatorErrorType Error;
} HTTPCallbackContext;


/**
 * Concatenates null-terminated strings from strArray into one result buffer. Result must be freed by the caller.
 *
 * @param strArray array of null-terminated mime types
 * @param arrayCount number of items in strArray
 * @param szTerminator string to be appended to each string value (e.g. +xml or +json)
 * @return a null-terminated string with mime types separated by a comma
 */
static char *ConcatenateAcceptValues(const char **strArray, size_t arrayCount, const char *szTerminator);
static void DataCallback(CreatorHTTPRequest request, void *callbackContext, const char *sData, size_t dataLength);
static void FinishCallback(CreatorHTTPRequest request, void *callbackContext, CreatorHTTPError error);
static void HeaderCallback(CreatorHTTPRequest request, void *callbackContext, const char *headerName, size_t nameLength, const char *headerValue, size_t valueLength);
static bool MakeOAuthSignature(const char *httpMethod, const char *url, char *dest, size_t destSize);
static void ResultCallback(CreatorHTTPRequest request, void *callbackContext, unsigned short httpResult);
static char *UnescapeUrl(char *url);


static char *nstrstr(const char *haystack, const char *needle, size_t haystackLength)
{
    size_t needle_length = strlen(needle);
    size_t index;

    for (index = 0; index < haystackLength; index++)
    {
        if (index + needle_length > haystackLength)
        {
            return NULL;
        }

        if (haystack[index] == needle[0])
        {
			if (strncmp(&haystack[index], needle, needle_length) == 0)
			{
				return (char *)&haystack[index];
			}
        }
    }
    return NULL;
}


char *CreatorHTTPMethod_ToString(CreatorHTTPMethod method)
{
	switch (method)
	{
		case CreatorHTTPMethod_Delete:
			return "DELETE";
		case CreatorHTTPMethod_Get:
			return "GET";
		case CreatorHTTPMethod_Post:
			return "POST";
		case CreatorHTTPMethod_Put:
			return "PUT";
		case CreatorHTTPMethod_Head:
			return "HEAD";
		case CreatorHTTPMethod_NotSet:
			break;
	}
	return "";
}

void CreatorHTTP_AddAuthorizationHeader(CreatorMemoryManager memoryManager, CreatorHTTPRequest request, CreatorHTTPMethod httpMethod, const char *url)
{
	if (CreatorThread_GetUseOAuth())
	{
		char oAuthSignature[512] = "OAuth ";
		size_t oauthPrefixLength, authorizationLength;
		oauthPrefixLength = 6;//strnlen(oAuthSignature, sizeof(oAuthSignature));
		if (MakeOAuthSignature(CreatorHTTPMethod_ToString(httpMethod), url, oAuthSignature + oauthPrefixLength, sizeof(oAuthSignature) - oauthPrefixLength))
		{
			authorizationLength = strlen(oAuthSignature);//strnlen(oAuthSignature, sizeof(oAuthSignature) - 1);
			/* append ,session_token="token" to Authorization header when available */
			CreatorToken sessionToken = CreatorClient_GetSessionToken();
			if (CreatorThread_GetLastError() == CreatorError_NoError)
			{
				if (sessionToken)
				{
					Creator_Log(CreatorLogLevel_Debug, "HTTP %p using session token [%s]", request, sessionToken);
					//FIXME these strnXXX calls are not using the right length...
					strncpy(oAuthSignature + authorizationLength, ",session_token=\"", sizeof(oAuthSignature) - 1 - authorizationLength);
					authorizationLength += sizeof(",session_token=\"") - 1;
					strncpy(oAuthSignature + authorizationLength, sessionToken, sizeof(oAuthSignature) - 1 - authorizationLength);
					strncat(oAuthSignature + authorizationLength, "\"", sizeof(oAuthSignature) - 1 - authorizationLength);
				}
				CreatorHTTPRequest_AddHeader(request, "Authorization", oAuthSignature);
			}
			if (sessionToken)
				CreatorToken_Free(&sessionToken);
		}
	}
}

bool CreatorHTTP_Call(CreatorMemoryManager memoryManager, CreatorHTTPMethod httpMethod, char *url, CreatorType expectedType, CreatorObject *pResponse, CreatorObject bodyData, CreatorHTTPStatus *status)
{
	bool result = false;
	CreatorThread_ClearLastError();
	if (pResponse)
		*pResponse = NULL;
	url  = UnescapeUrl(url);
	if (memoryManager && url)
	{
		if (httpMethod == CreatorHTTPMethod_Get)
		{
			CreatorObject cachedValue = CreatorCache_Get(url);
			if (cachedValue)
			{
				if (pResponse)
				{
					*pResponse = CreatorXMLDeserialiser_NewObject(expectedType);
					CreatorObject_CopyFrom(*pResponse, cachedValue);
					if (*pResponse)
					{
						CreatorMemoryManager_AttachObject(memoryManager, *pResponse);
						//CreatorObject_SetJob(*pResponse,memoryManager);
					}
				}
				if (status)
				{
					*status = CreatorHTTPStatus_OK; //Return HTTP status 200 if found in cache.
				}
				Creator_MemFree((void **)&url);
				return true;
			}
		}

		CreatorHTTPMethod actualMethod = httpMethod;
		//use X-HTTP-Method-Override
		if (httpMethod == CreatorHTTPMethod_Put || httpMethod == CreatorHTTPMethod_Delete)
		{
			httpMethod = CreatorHTTPMethod_Post;
		}

		//prepare context for HTTP call
		HTTPCallbackContext httpContext;
		memset(&httpContext, 0, sizeof(httpContext));

		httpContext.MemoryManager = memoryManager;
		httpContext.Success = true;
		httpContext.Semaphore = CreatorSemaphore_New(1, 1);
		httpContext.ExpectedType = expectedType;
		httpContext.ResponseType = CreatorType__Unknown;
		httpContext.IsSuccessResponse = false;
		httpContext.IsBadRequestResponse = false;
		httpContext.HasParserError = false;
		httpContext.Expires = 0;
		httpContext.Status = CreatorHTTPStatus_NotSet;
		httpContext.Error = CreatorError_NoError;

		httpContext.Request = CreatorHTTPRequest_New(actualMethod, url, ResultCallback, HeaderCallback, DataCallback, FinishCallback, &httpContext);

		if (httpContext.Semaphore && httpContext.Request)
		{
			Creator_Log(CreatorLogLevel_Debug, "HTTP %p will handle %s %s", httpContext.Request, CreatorHTTPMethod_ToString(httpMethod), url);

			//add X-HTTP-Method-Override
			if (httpMethod != actualMethod)
			{
				CreatorHTTPRequest_AddHeader(httpContext.Request, "X-HTTP-Method-Override", CreatorHTTPMethod_ToString(httpMethod));
			}

			const char *locale = CreatorClient_GetLocale();
			if (locale && strlen(locale))
			{
				CreatorHTTPRequest_AddHeader(httpContext.Request, "X-Culture", locale);
			}

			const char *bodyContentType = CreatorClient_GetBodyContentType();
			if (bodyContentType && strlen(bodyContentType))
			{
				if (!strstr(bodyContentType,"xml"))
				{
					CreatorHTTPRequest_AddHeader(httpContext.Request, "X-Body-Content-Type", bodyContentType);
				}
			}

			char requestId[65];
			snprintf(requestId, sizeof(requestId), "%lX", (unsigned long) httpContext.Request);
			CreatorHTTPRequest_AddHeader(httpContext.Request, "X-Client-RequestId", requestId);

			/* ??? TODO add Accept-Language ??? */

			//set up oAuth authorization
			CreatorHTTP_AddAuthorizationHeader(memoryManager, httpContext.Request, actualMethod, url);

			//add accept header

			const char *mimes[4] =
			{
					(httpContext.ExpectedType != CreatorType__Unknown) ? CreatorXMLDeserialiser_GetMIMEType(httpContext.ExpectedType) : "",
					CreatorXMLDeserialiser_GetMIMEType(CreatorType_Error),
					CreatorXMLDeserialiser_GetMIMEType( CreatorType_BadRequestResponse),
					"*/*"
			};

			char *acceptedMimeTypes = ConcatenateAcceptValues(mimes, sizeof(mimes) / sizeof(*mimes), "+xml");
			if (acceptedMimeTypes)
			{
				CreatorHTTPRequest_AddHeader(httpContext.Request, "Accept", acceptedMimeTypes);
				Creator_MemFree((void **)&acceptedMimeTypes);
			}

			if (httpContext.ExpectedType != CreatorType__Unknown)
				Creator_MemFree((void **) &mimes[0]);
			Creator_MemFree((void **) &mimes[1]);
			Creator_MemFree((void **) &mimes[2]);

			/* handle body */
			if (bodyData)
			{
				Creator_Assert(actualMethod != CreatorHTTPMethod_Get, "HTTP %p: GET does not allow a request body", httpContext.Request);
				// Note: some HTTP libs need all headers to be set before the data
				char contentType[256];
				char *mimeType = CreatorXMLDeserialiser_GetMIMEType(CreatorObject_GetType(bodyData));
				strncpy(contentType, mimeType, sizeof(contentType));
				Creator_MemFree((void **)&mimeType);
				strcat(contentType, "+xml");
				CreatorHTTPRequest_AddHeader(httpContext.Request, "Content-Type", contentType);
				int bodySize = 0;
				char *body = CreatorObject_SerialiseToXML(bodyData, &bodySize);
				if (body)
				{
					CreatorHTTPRequest_SetBody(httpContext.Request, body, bodySize);
					Creator_MemFree((void **)&body);
				}
			}

			if (CreatorThread_GetLastError() == CreatorError_NoError)
			{
				int attemptCount = 0;
				do
				{
					httpContext.Error = CreatorError_NoError;
					CreatorHTTPRequest_Send(httpContext.Request);

					/* wait for lock availability (data received and parsed) */
					CreatorSemaphore_Wait(httpContext.Semaphore, 1);
					attemptCount += 1;
				} while ((attemptCount < 2) && (httpContext.Error == CreatorError_Network));
				if (httpContext.Error != CreatorError_NoError)
					CreatorThread_SetError(httpContext.Error);
			}

			if (httpContext.IsSuccessResponse && !httpContext.HasParserError)
			{
				Creator_Log(CreatorLogLevel_Info, "HTTP %s %s response %u", CreatorHTTPMethod_ToString(httpMethod), url, httpContext.Status);
			}

			if (httpContext.Deserialiser)
			{
				CreatorObject response = CreatorXMLDeserialiser_GetObject(httpContext.Deserialiser);
				if (response)
				{
					if ((httpMethod == CreatorHTTPMethod_Get) && (httpContext.ResponseType == httpContext.ExpectedType))
					{
						CreatorXMLDeserialiser_SetSelfLink(httpContext.Deserialiser, response, url);
					}
					CreatorMemoryManager_AttachObject(memoryManager, response);
				}
				if (pResponse)
				{
					*pResponse = response;
				}
				CreatorXMLDeserialiser_Free(&httpContext.Deserialiser);
			}

			//notify cache manager of the result
			if (httpContext.IsSuccessResponse && !httpContext.HasParserError)
			{
				if ((httpMethod == CreatorHTTPMethod_Get) && CreatorThread_GetUseOAuth())
				{
					if ((httpContext.ResponseType == httpContext.ExpectedType) && (httpContext.Expires > 0))
					{
						if (pResponse)
							CreatorCache_Set(url, *pResponse, httpContext.Expires);
					}
				}
			}

			if (!httpContext.IsSuccessResponse)
			{
				if (!httpContext.HasParserError && httpContext.ResponseType == CreatorType_BadRequestResponse)
				{
					if (pResponse)
					{
						CreatorBadRequestResponse errorResponse = (CreatorBadRequestResponse) *pResponse;
						int errorCode = CreatorBadRequestResponse_GetErrorCode(errorResponse);
						CreatorErrorType errorType =
						        ((errorCode - 1 + (int) CreatorError_BadRequest_Min >= CreatorError_BadRequest_Unknown) || (errorCode <= 0)) ?
						                CreatorError_BadRequest_Unknown : (CreatorErrorType) (errorCode - 1 + (int) CreatorError_BadRequest_Min);
						CreatorThread_SetError(errorType);
						Creator_Log(CreatorLogLevel_Error, "HTTP %s %s response %u - failed with [%d]", CreatorHTTPMethod_ToString(httpMethod), url, httpContext.Status, errorType);
					}
					else
					{
						CreatorThread_SetError(CreatorError_BadRequest_Unknown);
					}
				}
				else if (httpContext.Error == CreatorError_Server)
				{
					Creator_Log(CreatorLogLevel_Error, "HTTP %s %s response %u - failed (internal server error)", CreatorHTTPMethod_ToString(httpMethod), url, httpContext.Status);
				}
				else
				{
					Creator_Log(CreatorLogLevel_Error, "HTTP %s %s response %u - failed (no content returned)", CreatorHTTPMethod_ToString(httpMethod), url, httpContext.Status);
					if (CreatorThread_GetLastError() == CreatorError_NoError)
					{
						CreatorThread_SetError((httpContext.IsBadRequestResponse) ? CreatorError_BadRequest_Unknown : CreatorError_InvalidArgument);
					}
					httpContext.Success = false;
				}
			}
		}
		else
		{
			/* error */
			//Creator_Log(CreatorLogLevel_Error, "HTTP %p (%s %s) failed to start: parser instance %p, semaphore %p", httpContext.Request, CreatorHTTPMethod_ToString(httpMethod), url, httpContext.parsingContext.parserInstance, httpContext.Semaphore);
			Creator_Log(CreatorLogLevel_Error, "HTTP %p (%s %s) failed to start", httpContext.Request, CreatorHTTPMethod_ToString(httpMethod), url);
			CreatorThread_SetError(CreatorError_Internal);
			httpContext.Success = false;
		}

		if (httpContext.Request)
		{
			CreatorHTTPRequest_Free(&httpContext.Request);
		}
		if (httpContext.Semaphore)
		{
			CreatorSemaphore_Free(&httpContext.Semaphore);
		}
		if (status)
			*status = httpContext.Status;
		result = httpContext.Success;
	}
	if (url)
		Creator_MemFree((void **)&url);
	return result;
}



static char *ConcatenateAcceptValues(const char **strArray, size_t arrayCount, const char *terminator)
{
	size_t stringLengths[arrayCount];
	size_t index;
	size_t totalLength = 1;
	size_t terminatorLength = strlen(terminator);
	for (index = 0 ; index < arrayCount; ++index)
	{
		stringLengths[index] = strlen(strArray[index]);
		//protect against empty strings
		if (stringLengths[index])
		{
			totalLength += stringLengths[index] + 2 + terminatorLength;
		}
	}
	if (totalLength > 1)
	{
		totalLength -= 2 + terminatorLength;
	}
	char *acceptedMimeTypes = (char*)Creator_MemAlloc(totalLength);
	if (!acceptedMimeTypes)
	{
		return NULL;
	}
	size_t writtenLength = 0;
	for (index = 0 ; index < arrayCount; ++index)
	{
		//protect against empty strings
		if (stringLengths[index])
		{
			memcpy(acceptedMimeTypes + writtenLength, strArray[index], stringLengths[index]);
			writtenLength += stringLengths[index];
			if (index < arrayCount - 1)
			{
				memcpy(acceptedMimeTypes + writtenLength, terminator, terminatorLength);
				writtenLength += terminatorLength;
				acceptedMimeTypes[writtenLength++] = ',';
				acceptedMimeTypes[writtenLength++] = ' ';
			}
		}
	}
	acceptedMimeTypes[writtenLength++] = '\0';
	Creator_Assert(writtenLength == totalLength, "algo error while building Accept header value");
	return acceptedMimeTypes;
}

static void DataCallback(CreatorHTTPRequest request, void *callbackContext, const char *sData, size_t dataLength)
{
	HTTPCallbackContext *httpContext = (HTTPCallbackContext*)callbackContext;
	if (httpContext->ResponseType == CreatorType__Unknown)
	{
		Creator_Log(CreatorLogLevel_Debug, "HTTP %p: data received: %s", httpContext->Request, dataLength, sData);
	}
	else if (httpContext->Deserialiser)
	{
		if (!CreatorXMLDeserialiser_Read(httpContext->Deserialiser, sData, dataLength, false))
		{
			httpContext->Error = CreatorError_Internal;
			httpContext->HasParserError = true;
			//Creator_Log(CreatorLogLevel_Debug, "HTTP %p: XML status %d, failed to parse data: %*s", httpContext->Request, xmlStatus, dataLength, sData);
			Creator_Log(CreatorLogLevel_Debug, "HTTP %p: len=%d, failed to parse data: %s", httpContext->Request, dataLength, sData);
		}
	}
	else if (httpContext->Error != CreatorError_Server)
	{
		Creator_Log(CreatorLogLevel_Warning, "HTTP %p: len=%d, receiving unexpected data: %s", httpContext->Request, dataLength, sData);
	}
}

static void FinishCallback(CreatorHTTPRequest request, void *callbackContext, CreatorHTTPError error)
{
	HTTPCallbackContext *httpContext = (HTTPCallbackContext*)callbackContext;
	if (error != CreatorHTTPError_None)
	{
		CreatorErrorType errorKind = CreatorError_Network;
		if (error == CreatorHTTPError_Timeout)
		{
			errorKind = CreatorError_Timeout;
		}
		httpContext->Error = errorKind;
		httpContext->Success = false;
	}
	else if (httpContext->Deserialiser)
	{
		if (CreatorXMLDeserialiser_InParsingState(httpContext->Deserialiser))
		{
			if (!CreatorXMLDeserialiser_Read(httpContext->Deserialiser, "", 0, true))
			{
				httpContext->Error = CreatorError_Internal;
				httpContext->HasParserError = true;
				//Creator_Log(CreatorLogLevel_Debug, "HTTP %p: XML status %d, failed to parse data", httpContext->Request, xmlStatus);
				Creator_Log(CreatorLogLevel_Debug, "HTTP %p: failed to parse data", httpContext->Request);
			}
		}
	}
	CreatorSemaphore_Release(httpContext->Semaphore, 1);
}

static void HeaderCallback(CreatorHTTPRequest request, void *callbackContext, const char *headerName, size_t nameLength, const char *headerValue, size_t valueLength)
{
	HTTPCallbackContext *httpContext = (HTTPCallbackContext*)callbackContext;
	/* setup the parser for the right content-type */
	if (nameLength == sizeof("Content-Type") - 1 && strncmp(headerName, "Content-Type", sizeof("Content-Type") - 1) == 0)
	{
		CreatorType contentType = CreatorType__Unknown;
		/* go through ;-separated mime types and match one of them against the ones we know */
		int mimeStart = 0, mimeStop = -1;
		size_t index;
		bool mimeFound = false;
		for (index = 0; index < valueLength; ++index)
		{
			if (headerValue[index] == ' ')
			{
				if (!mimeFound)
				{
					mimeStart = index+1;
				}
				else if (mimeStop == -1)
				{
					mimeStop = index;
				}
			}
			else if (headerValue[index] == ';')
			{
				if (mimeStop == -1)
				{
					mimeStop = index;
				}
				Creator_Assert(mimeStop > mimeStart, "content-type parsing error");
				if (mimeStart > mimeStop)
				{
					break;
				}
				size_t mimeLength = (size_t)mimeStop - (size_t)mimeStart;
				/* remove the +xml prefix */
				if (strncmp(headerValue + mimeStop - 4, "+xml", 4) == 0)
				{
					mimeLength -= 4;
				}
				contentType = CreatorXMLDeserialiser_GetCreatorType((char *)&headerValue[mimeStart], mimeLength);
				if (contentType != CreatorType__Unknown)
				{
					break;
				}
				mimeStart = -1;
				mimeFound = false;
				mimeStop = -1;
			}
			else
			{
				if (mimeStart == -1)
				{
					mimeStart = index;
				}
				mimeFound = true;
			}
		}
		httpContext->ResponseType = contentType;
		if ((contentType == CreatorType_BadRequestResponse) || (httpContext->ExpectedType == httpContext->ResponseType))
		{
			httpContext->Deserialiser = CreatorXMLDeserialiser_New(contentType);
		}
		else if ((httpContext->ExpectedType != httpContext->ResponseType) && (httpContext->ExpectedType != CreatorType__Unknown) && (httpContext->Error != CreatorError_Server))
		{
			// Note: can't print headerValue directly because it's not null terminated
			//Creator_Log(CreatorLogLevel_Error, "HTTP %p: Expected type %d, received %d (%s)", httpContext->Request, httpContext->ExpectedType, contentType, headerValue);
			Creator_Log(CreatorLogLevel_Error, "HTTP %p: Expected type %d, received %d", httpContext->Request, httpContext->ExpectedType, contentType);
			if (httpContext->Error == CreatorError_NoError)
				httpContext->Error = CreatorError_Internal;
		}

//		if (httpContext->targetDataHandler && httpContext->ResponseType != CreatorType__Unknown) {
//			const XMLParsingHandler *pTargetHandler = CreatorXMLParsing_GetHandlerForType(contentType);
//			if (!pTargetHandler || !pTargetHandler->startHandler) {
//				Creator_Log(CreatorLogLevel_Error, "HTTP %p: parsing handler for type %d (%*s) is not available", httpContext->Request, contentType, headerValue);
//				Creator_Assert(0, "Did you register the relevant license? e.g. CreatorRegisterLicenseCore()");
//			}
//			CreatorXMLHandlerStack_push(httpContext->parsingContext.handlerStack, pTargetHandler, &httpContext->parsingContext);
//		}
	}
//	else if (nameLength == sizeof("Expires") - 1
//			&& strncmp(headerName, "Expires", sizeof("Expires") - 1) == 0)
//	{
//		struct tm tm;
//		memset(&tm, 0, sizeof(struct tm));
//		char *pTimeEnd = strptime(headerValue, "%a, %d %b %Y %H:%M:%S %Z", &tm);
//		if (!pTimeEnd) {
//			Creator_Log(CreatorLogLevel_Warning, "Failed to parse time %*s", valueLength, headerValue);
//		}
//		else if ((unsigned)(pTimeEnd - headerValue) == valueLength) {
//			 time_t localTime = mktime(&tm);
//			 httpContext->Expires = localTime + timezoneOffset();
//		}
//	}
	else if (nameLength == sizeof("Cache-Control") - 1 && strncmp(headerName, "Cache-Control", sizeof("Cache-Control") - 1) == 0)
	{
		char* position = nstrstr(headerValue, "max-age=", valueLength);
		if (position)
		{
			position += 8;
			int expirySeconds = (int)strtol(position, NULL, 10);
			httpContext->Expires = CreatorServerTime_GetServerTime() + expirySeconds;
		}
	}
}

bool MakeOAuthSignature(const char *httpMethod, const char *url, char *dest, size_t destSize)
{
	bool result = false;
	if (CreatorServerTime_IsSynchronized())
	{
		char *signedUrl = oauth_sign_url_creator (url, OA_HMAC, httpMethod, CreatorClient_GetOAuthKey(), CreatorClient_GetOAuthSecret());
		if (signedUrl)
		{
			int signedUrlLength = strlen(signedUrl);
			if ( signedUrlLength > (destSize-1))
			{
				CreatorThread_SetError(CreatorError_Internal); //"Failed to sign url (pDst too short)"
			}
			else
			{
				memcpy(dest, signedUrl, signedUrlLength+1);
				result = true;
			}
			Creator_MemFree((void **)&signedUrl);
		}
		else
		{
			CreatorThread_SetError(CreatorError_Internal); //"Failed to sign url"
		}
	}
	else
	{
		CreatorThread_SetError(CreatorError_ServerTimeSync);
	}
	return result;
}


static void ResultCallback(CreatorHTTPRequest request, void *callbackContext, unsigned short httpResult)
{
	HTTPCallbackContext *httpContext = (HTTPCallbackContext*)callbackContext;
	Creator_Log(CreatorLogLevel_Debug, "HTTP %p received response %u", httpContext->Request, httpResult);
	httpContext->Status = (CreatorHTTPStatus) httpResult;
	unsigned short codeCat = httpResult / 100;
	if (codeCat != 2)
	{
		//not a HTTP 2xx success
		httpContext->IsSuccessResponse = false;

		CreatorErrorType newError = CreatorError_NoError;
		switch(httpResult)
		{
			case 400:
				//there could be a BadRequest to parse, don't set the error right away to be as specific as possible
				httpContext->IsBadRequestResponse = true;
				break;
			case 401: case 403: newError = CreatorError_Unauthorised; break;
			case 404: newError = CreatorError_ResourceNotFound; break;
			case 405: newError = CreatorError_Internal; break;
			case 406: case 501: newError = CreatorError_VersionConflict; break;
			case 409: newError = CreatorError_Conflict; break;
			case 410: newError = CreatorError_Removed; break;
			case 500: newError = CreatorError_Server; break;
			case 503: case 507: newError = CreatorError_ServerBusy; break;
			case 408: case 504: newError = CreatorError_Timeout; break;
			default:
				//unexpected HTTP response code
				newError = CreatorError_Server;
				switch (codeCat)
				{
					//3xx are not expected, so the client is not handling it as it should
					case 3: newError = CreatorError_Internal; break;
					//4xx are client errors
					case 4: newError = CreatorError_Internal; break;
					//5xx are server errors
					case 5: newError = CreatorError_Server; break;
				}
				break;
		}
		if (newError != CreatorError_NoError)
		{
			httpContext->Error = newError;
		}
	}
	else
	{
		httpContext->IsSuccessResponse = true;
	}
}

static char *UnescapeUrl(char *url)
{
	char *result = NULL;
	if (url)
	{
		char *startOfQuery = NULL;
		int countOfSpaces = 0;
		char *position = url;
		while(*position != '\0')
		{
			if (startOfQuery)
			{
				if (*position == '+')
					countOfSpaces++;
				if (*position == '%')
				{
					position[1] = toupper(position[1]);
					position[2] = toupper(position[2]);
				}
			}
			else
			{
				if (*position == '?')
					startOfQuery = position;
			}
			position++;
		}
		if (countOfSpaces > 0)
		{
			result = (char *)Creator_MemAlloc(position-url+sizeof(char)+ (countOfSpaces*2*sizeof(char)));
			if (result)
			{
				memcpy(result,url,startOfQuery-url);
				char *dest = result + (startOfQuery-url);
				position = startOfQuery;
				while(*position != '\0')
				{
					if (*position == '+')
					{
						*dest ='%';
						dest++;
						*dest ='2';
						dest++;
						*dest ='0';
					}
					else
					{
						*dest = *position;
					}
					position++;
					dest++;
				}
				*dest ='\0';
			}
		}
		else
			result = CreatorString_Duplicate(url);
	}
	return result;
}
