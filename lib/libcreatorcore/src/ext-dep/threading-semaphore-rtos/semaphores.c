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

#ifdef FREERTOS

#include "FreeRTOS.h"
#include "semphr.h"

#include "creator/core/creator_timer.h"
#include "creator/core/creator_threading.h"
#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_debug.h"

CreatorSemaphore CreatorSemaphore_New(uint tokensTotal, uint tokensTaken)
{
	xSemaphoreHandle result;
	Creator_Assert(tokensTaken <= tokensTotal, "Bad initial number of tokens");
	if (tokensTaken > tokensTotal)
	{
		tokensTaken = tokensTotal;
	}
    result = xSemaphoreCreateCounting(tokensTotal, (tokensTotal - tokensTaken));
	return result;
}

void CreatorSemaphore_Wait(CreatorSemaphore self, uint tokens)
{
	xSemaphoreHandle semaphore = (xSemaphoreHandle)self;
	uint index;
	for (index = 0; index < tokens; ++index)
	{
		BaseType_t result;
		do
		{
			result = xSemaphoreTake(semaphore, portMAX_DELAY);
		}while (result != pdTRUE);
	}
}


bool CreatorSemaphore_WaitFor(CreatorSemaphore self, uint tokens, uint milliseconds)
{
	bool result = true;
	xSemaphoreHandle semaphore = (xSemaphoreHandle)self;
	uint index;
	for (index = 0; index < tokens; ++index)
	{
		if (xSemaphoreTake(semaphore,(milliseconds * CreatorTimer_GetTicksPerSecond())/1000) != pdTRUE)
		{
			if (index > 0)
				CreatorSemaphore_Release(semaphore, index-1);
			result =  false;
			break;
		}
	}
	return result;
}

void CreatorSemaphore_Release(CreatorSemaphore self, uint tokens) {
	xSemaphoreHandle semaphore = (xSemaphoreHandle)self;
	uint index;
	for (index = 0; index < tokens; ++index)
	{
		xSemaphoreGive(semaphore);
	}
}

void CreatorSemaphore_Free(CreatorSemaphore *self)
{
	if (self && *self)
	{
		xSemaphoreHandle semaphore = (xSemaphoreHandle)*self;
		vSemaphoreDelete(semaphore);
		*self = NULL;
	}
}

#endif
