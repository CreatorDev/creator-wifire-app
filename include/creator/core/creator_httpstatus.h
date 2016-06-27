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

/*! \file creator_httpstatus.h
 *  \brief LibCreatorCore .
 */


#ifndef CREATOR_HTTPSTATUS_H_
#define CREATOR_HTTPSTATUS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    CreatorHTTPStatus_NotSet = 0,
    CreatorHTTPStatus_Continue = 100,
    CreatorHTTPStatus_SwitchingProtocols = 101,
    CreatorHTTPStatus_OK = 200,
    CreatorHTTPStatus_Created = 201,
    CreatorHTTPStatus_Accepted = 202,
    CreatorHTTPStatus_NonAuthoritativeInformation = 203,
    CreatorHTTPStatus_NoContent = 204,
    CreatorHTTPStatus_ResetContent = 205,
    CreatorHTTPStatus_Partialcontent = 206,
    CreatorHTTPStatus_MultipleChoices = 300,
    CreatorHTTPStatus_MovedPermanently = 301,
    CreatorHTTPStatus_MovedTemporarily = 302,
    CreatorHTTPStatus_SeeOther = 303,
    CreatorHTTPStatus_NotModified = 304,
    CreatorHTTPStatus_UseProxy = 305,
    CreatorHTTPStatus_BadRequest = 400,
    CreatorHTTPStatus_Unauthorized = 401,
    CreatorHTTPStatus_PaymentRequired = 402,
    CreatorHTTPStatus_Forbidden = 403,
    CreatorHTTPStatus_NotFound = 404,
    CreatorHTTPStatus_MethodNotAllowed = 405,
    CreatorHTTPStatus_NotAcceptable = 406,
    CreatorHTTPStatus_ProxyAuthenticationRequired = 407,
    CreatorHTTPStatus_RequestTimeout = 408,
    CreatorHTTPStatus_Conflict = 409,
    CreatorHTTPStatus_Gone = 410,
    CreatorHTTPStatus_LengthRequired = 411,
    CreatorHTTPStatus_PreconditionFailed = 412,
    CreatorHTTPStatus_RequestEntityTooLarge = 413,
    CreatorHTTPStatus_RequestURITooLarge = 414,
    CreatorHTTPStatus_UnsupportedMediaType = 415,
    CreatorHTTPStatus_InternalServerError = 500,
    CreatorHTTPStatus_NotImplemented = 501,
    CreatorHTTPStatus_BadGateway = 502,
    CreatorHTTPStatus_ServiceUnavailable = 503,
    CreatorHTTPStatus_GatewayTimeout = 504,
    CreatorHTTPStatus_HttpVersionNotSupported = 505
} CreatorHTTPStatus;


#ifdef __cplusplus
}
#endif

#endif /* CREATOR_HTTPSTATUS_H_ */
