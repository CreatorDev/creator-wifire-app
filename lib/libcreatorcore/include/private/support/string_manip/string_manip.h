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


#ifndef STRING_MANIP_H_
#define STRING_MANIP_H_

#include <stdlib.h>
#include <stdbool.h>

/**
 * Method that can encodes src into dest.
 * Typically, one would:
 * 		- call it a first time with dest == NULL and a valid pDstSize.
 * 		- allocate a buffer of at least *pDstSize bytes
 * 		- call it again with a valid dest and pDstSize == NULL
 *
 * @param src string to encode
 * @param dest memory buffer to hold the encoded string. can be NULL
 * @param pDstSize pointer to variable which will hold the encoded length. does not include \0. can be NULL
 * @return bool if the encoding succeeded, false otherwise
 */
typedef bool (*StringManip_EncodingFn)(const char *src, char *dest, size_t *pDstSize);

/**
 * Encoder that keeps the source string as it is
 * @see StringManip_EncodingFn
 */
bool StringManip_DontEncode(const char *src, char *dest, size_t *pDstSize);


char *StringManip_EscapeCharacters(const char *src, const char *szEscapableCharacters, const char escapeCharacter);

void StringManip_InplaceReplace(char *haystack, size_t *haystackLength, char *szPin, const char *szReplacement, StringManip_EncodingFn replacementEncoder);

void StringManip_StrPad(char *text, size_t padLength, char padChar);

/**
 * Inserts a string into src where c is first found. If C is not found, the string is appended at the end
 * @param src source string to modify
 * @param c character to look for
 * @param szInsert string to insert
 * @return a dynamically-allocated string that must be freed by the caller
 */
char *StringManip_InsertAt(const char *src, char c, const char *szInsert);

/**
 * Builds a query string following the format "format(term, value1) connector format(term, value2) ...".
 * Can optionally apply escaping on values and term.
 *
 * e.g. StringManip_buildQueryString("genreID", {"id1", "id2", "+super"}, 3, " OR ", "!id:\"!val\"", encoder)
 * will produce:
 * 		genreID:"id1" OR genreID:"id2" OR genreID:"\+super"
 *
 * @param szTerm term against which each value is matched
 * @param valuesArray array of values
 * @param valuesCount number of items in the array of values
 * @param valueBinder connector to use between each term,value pair (e.g. AND, OR)
 * @param formatTemplate string with a placeholder !id for the term and !val for the value
 * @param valueEncoder encoding method which will be used to encode both term and values. can be NULL for no encoding
 * @return a dynamically-allocated query string, to be freed with Creator_MemFree
 */
char *StringManip_BuildQueryString(const char *szTerm, const char **valuesArray, size_t valuesCount, const char *valueBinder, const char *formatTemplate, StringManip_EncodingFn valueEncoder);

#endif /* STRING_MANIP_H_ */
