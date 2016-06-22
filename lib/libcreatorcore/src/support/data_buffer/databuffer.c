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

#include <stddef.h>
#include <string.h>

#include "data_buffer.h"

#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_debug.h"


struct DataBufferImpl
{
	char *pBuffer;
	size_t dataSize;
	size_t bufferSize;
	bool bCorrupt;
};

static void DataBuffer_Resize(DataBuffer *pBuffer, size_t newMinSize);


DataBuffer *DataBuffer_New()
{
	DataBuffer *pResult = Creator_MemAlloc(sizeof(DataBuffer));
	if (pResult) {
		pResult->pBuffer = NULL;
		pResult->dataSize = 0;
		pResult->bufferSize = 0;
		pResult->bCorrupt = false;
	}
	return pResult;
}

bool DataBuffer_Write(DataBuffer *pBuffer, const void *pData, size_t dataSize)
{
	if (!pBuffer) {
		return false;
	}
	if (pBuffer->dataSize + dataSize > pBuffer->bufferSize) {
		DataBuffer_Resize(pBuffer, pBuffer->dataSize + dataSize);
	}
	if (!pBuffer->bCorrupt) {
		memcpy(pBuffer->pBuffer + pBuffer->dataSize, pData, dataSize);
		pBuffer->dataSize += dataSize;
		return true;
	}
	return false;
}

bool DataBuffer_WriteCString(DataBuffer *pBuffer, const char *szData)
{
	if (!pBuffer) {
		return false;
	}
	size_t dataSize = (szData)?strlen(szData):0;
	//FIXME optimize this impl
	return DataBuffer_Write(pBuffer, szData, dataSize);
}

void *DataBuffer_GetBufferData(DataBuffer *pBuffer, bool bTakeOwnership)
{
	if (!pBuffer || pBuffer->bCorrupt) {
		return NULL;
	}
	if (bTakeOwnership) {
		void *pResult = pBuffer->pBuffer;
		pBuffer->pBuffer = NULL;
		pBuffer->bufferSize = 0;
		pBuffer->dataSize = 0;
		return pResult;
	}
	else if (pBuffer->dataSize > 0) {
		void *pResult = Creator_MemAlloc(pBuffer->dataSize);
		if (pResult) {
			memcpy(pResult, pBuffer->pBuffer, pBuffer->dataSize);
		}
		return pResult;
	}
	return NULL;
}

size_t DataBuffer_GetDataSize(DataBuffer *pBuffer)
{
	if (!pBuffer || pBuffer->bCorrupt) {
		return 0;
	}
	return pBuffer->dataSize;
}

void DataBuffer_Free(DataBuffer **self)
{
	if (self && *self)
	{
		DataBuffer *buffer = *self;
		if (buffer->pBuffer) {
			Creator_MemFree((void **)&buffer->pBuffer);
		}
		Creator_MemFree((void **)self);
	}
}

#define BUFFER_MINIMUM_SIZE (2048)
#define MAX(a,b) ((a < b)?b:a)

static void DataBuffer_Resize(DataBuffer *pBuffer, size_t newMinSize)
{
	if (pBuffer->bCorrupt) {
		return ;
	}
	Creator_Assert(pBuffer->bufferSize < newMinSize, "Calling DataBuffer_Resize unnecessarily");
	size_t newSize = MAX(BUFFER_MINIMUM_SIZE, MAX(newMinSize, pBuffer->bufferSize * 2));
	char *pNewBuffer;
	if (pBuffer->bufferSize) {
		pNewBuffer = Creator_MemRealloc(pBuffer->pBuffer, newSize);
	}
	else {
		pNewBuffer = Creator_MemAlloc(newSize);
	}
	if (pNewBuffer) {
		pBuffer->pBuffer = pNewBuffer;
		pBuffer->bufferSize = newSize;
	}
	else {
		pBuffer->bCorrupt = true;
	}
}
