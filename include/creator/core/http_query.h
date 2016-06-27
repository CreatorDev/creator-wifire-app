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

#ifndef CREATOR_HTTP_QUERY_PRIVATE_H_
#define CREATOR_HTTP_QUERY_PRIVATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "creator/core/base_types.h"
#include "creator/core/creator_httpmethod.h"

typedef struct _CreatorHTTPQuery *CreatorHTTPQuery;


/**
 * Append a suffix to the url, e.g. for suffix "email@company.com", "/email@company.com"
 *  will be appended to the base url and encoded properly
 * @param self
 * @param suffix
 */
bool CreatorHTTPQuery_AppendPathSuffix(CreatorHTTPQuery self, const char *suffix);
bool CreatorHTTPQuery_AppendPathSuffixBoolean(CreatorHTTPQuery self, bool value);
bool CreatorHTTPQuery_AppendPathSuffixChar(CreatorHTTPQuery self, char value);
bool CreatorHTTPQuery_AppendPathSuffixInt(CreatorHTTPQuery self, int value);

/**
 * Constructs a new url from the query parameters and base url
 *
 * @param self the query parameters handle
 * @return a newly-allocated null-terminated string of the result URL
 */
char *CreatorHTTPQuery_GenerateUrl(CreatorHTTPQuery self);

/**
 * Returns a dynamically-allocated string containing url path
 *
 * @param self
 * @return
 */
char *CreatorHTTPQuery_GetBaseUrl(CreatorHTTPQuery self);

/**
 * Returns a dynamically-allocated string containing the value for self query parameter
 *
 * @param self
 * @param param
 * @return
 */
char *CreatorHTTPQuery_GetQueryParameter(CreatorHTTPQuery self, const char *param);

CreatorHTTPQuery CreatorHTTPQuery_New(void);
CreatorHTTPQuery CreatorHTTPQuery_NewFromUrl(const char *url);

void CreatorHTTPQuery_SetBaseUrl(CreatorHTTPQuery self, const char *szBaseUrl);

void CreatorHTTPQuery_SetQueryParameter(CreatorHTTPQuery self, const char *param, const char *paramValue);

void CreatorHTTPQuery_Free(CreatorHTTPQuery *self);

#ifdef __cplusplus
}
#endif

#endif /* CREATOR_HTTP_QUERY_PRIVATE_H_ */
