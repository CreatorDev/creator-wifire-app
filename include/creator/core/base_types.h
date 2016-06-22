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
/** @file */

#ifndef CREATOR_CORE_BASE_TYPES_H_
#define CREATOR_CORE_BASE_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

typedef enum
{
	CreatorType__Unknown = 0,
	CreatorType_Array,
	CreatorType_Boolean,
	CreatorType_Char,
	CreatorType_Datetime,
	CreatorType_ID,
	CreatorType_Integer,
	CreatorType_Long,
	CreatorType_String,
	CreatorType_Timespan,
	CreatorType_Token,
	CreatorType_RawString,

	CreatorType__Max
} CreatorType;             // TODO - deprecate: use std types


typedef time_t CreatorDatetime;

typedef char* CreatorID;   // TODO - delete?

typedef long int CreatorTimespan;

typedef char* CreatorToken;

#ifndef	CREATOR_STDTYPES_INCLUDED

typedef	int8_t int8;
typedef	uint8_t	uint8;

typedef	int16_t	int16;
typedef	uint16_t uint16;

typedef	int32_t	int32;
typedef	uint32_t uint32;

typedef	int64_t int64;
typedef	uint64_t uint64;

typedef	unsigned char uchar;
typedef	unsigned short ushort;
typedef	unsigned int uint;
typedef	unsigned long ulong;

#define CREATOR_STDTYPES_INCLUDED
#endif

#ifdef __cplusplus
}
#endif

#endif /* CREATOR_CORE_BASE_TYPES_H_ */
