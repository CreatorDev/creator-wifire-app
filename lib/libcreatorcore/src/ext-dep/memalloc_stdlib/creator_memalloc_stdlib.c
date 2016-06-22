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

#ifndef FREERTOS

#include <stdlib.h>
#include <string.h>

#include "creator/core/creator_debug.h"
#include "creator/core/creator_memalloc.h"

void *Creator_MemAlloc(size_t size)
{
	void *result = malloc(size);
	//Creator_Log(CreatorLogLevel_Debug, "Allocating %ld bytes to %p", size, result);
	Creator_Assert(result != NULL, "Allocation of %ld bytes failed", size);
	return result;
}

void *Creator_MemCalloc(size_t blockCount, size_t blockSize)
{
	void *result = calloc(blockCount, blockSize);
	Creator_Assert(result != NULL, "Allocation of %ld bytes failed", blockCount * blockSize);
	return result;
}

void *Creator_MemRealloc(void *buffer, size_t size)
{
	void *result = realloc(buffer, size);
	//Creator_Log(CreatorLogLevel_Debug, "Reallocating %p with %ld bytes as %p", pBuf, size, result);
	Creator_Assert(result != NULL, "(Re)Allocation of %ld bytes failed", size);
	return result;
}

void Creator_MemFree(void **buffer)
{
	//Creator_Log(CreatorLogLevel_Debug, "Freeing %p", pBuf);
	if (*buffer)
        {
		free(*buffer);
		*buffer = NULL;
	}
}

void Creator_MemSafeFree(void *buffer)
{
	if (buffer)
	{
		free(buffer);
	}
}

#endif
