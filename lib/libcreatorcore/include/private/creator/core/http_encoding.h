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


#ifndef HTTP_ENCODING_H_
#define HTTP_ENCODING_H_

#include <stdbool.h>
#include <stddef.h>

/**
 * Encode or gives the encoded length of a path segment according to RFC3986. The result does not include a terminating \\0.
 *
 * @param value the null-terminated string to encode
 * @param dest either NULL if you want to know the encoded size only, or a pointer to a buffer that will hold the encoded result
 * @param destSize the available memory in that buffer
 * @param[out] encodedSize either NULL, or a pointer to the variable that will hold the size required for the encoded string.
 * @return true if successful, false otherwise
 */
bool CreatorHTTP_EncodePathSegmentRFC3986(const char *value, char *dest, size_t destSize, size_t *encodedSize);

/**
 * Decodes or gives the decoded length of a path segment according to RFC3986. The result does not include a terminating \\0.
 *
 * @param value a segment to decode, terminated by either '\\0', '?' or '/'
 * @param dest the destination buffer. can be NULL
 * @param destSize size of the destination buffer
 * @param[out] decodedSize size required to store the decoded segment
 * @return true if successful, false otherwise
 */
bool CreatorHTTP_DecodePathSegmentRFC3986(const char *value, char *dest, size_t destSize, size_t *decodedSize);

#endif /* HTTP_ENCODING_H_ */
