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


/*! \file creator_queue.c
 *  \brief LibCreatorCore .
 */

#include "creator/core/creator_queue.h"
#include "creator/core/base_types.h"
#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_threading.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

struct CreatorQueueImpl
{
    uint Size;
    uint Used;
    uint Head;
    uint Tail;
    CreatorSemaphore QueueLock;
    void **Items;
    CreatorSemaphore WaitForItem;
};

CreatorQueue CreatorQueue_New(unsigned initialCapacity)
{
    CreatorQueue result = NULL;
    result = Creator_MemAlloc(sizeof(struct CreatorQueueImpl));
    if (result)
    {
        if (initialCapacity == 0)
            initialCapacity = 10;
        result->Used = 0;
        result->Head = 0;
        result->Tail = 0;
        result->WaitForItem = NULL;
        result->QueueLock = CreatorSemaphore_New(1, 0);
        if (result->QueueLock)
        {
            size_t size = sizeof(void*) * initialCapacity;
            result->Items = Creator_MemAlloc(size);
            if (result->Items)
            {
                memset(result->Items, 0, size);
                result->Size = initialCapacity;
            }
            else
                result->Size = 0;
        }
        else
        {
            Creator_MemFree((void **)&result);
        }
    }
    return result;
}

CreatorQueue CreatorQueue_NewBlocking(unsigned initialCapacity)
{
    CreatorQueue result = CreatorQueue_New(initialCapacity);
    if (result)
    {
        result->WaitForItem = CreatorSemaphore_New(1, 1);
        if (!result->WaitForItem)
        {
            CreatorQueue_Free(&result);
        }
    }
    return result;
}

bool CreatorQueue_Enqueue(CreatorQueue self, void* item)
{
    bool result = false;
    if (self)
    {
        CreatorSemaphore_Wait(self->QueueLock, 1);
        int count = self->Used + 1;
        if (count > self->Size)
        {
            if (self->Head < self->Tail)
            {
                void* *newItems = Creator_MemRealloc(self->Items, (self->Size + 5) * sizeof(void*));
                if (newItems)
                {
                    self->Size += 5;
                    self->Items = newItems;
                }
            }
            else
            {
                void* *newItems = Creator_MemAlloc((self->Size + 5) * sizeof(void*));
                if (newItems)
                {
                    uint elementsInTopPart = (self->Size - self->Head);
                    memcpy(newItems, self->Items + self->Head, elementsInTopPart * sizeof(void*));
                    memcpy(newItems + elementsInTopPart, self->Items, self->Tail * sizeof(void*));
                    self->Head = 0;
                    self->Tail = self->Size;
                    self->Size += 5;
                    Creator_MemFree((void **)&self->Items);
                    self->Items = newItems;
                }
            }
        }
        if (count <= self->Size)
        {
            self->Items[self->Tail] = item;
            self->Used = count;
            self->Tail = (self->Tail + 1) % self->Size;
            result = true;
        }
        CreatorSemaphore_Release(self->QueueLock, 1);
        if (self->WaitForItem)
            CreatorSemaphore_Release(self->WaitForItem, 1);
    }
    return result;
}

void* CreatorQueue_Dequeue(CreatorQueue self)
{
    void* result = NULL;
    if (self)
    {
        bool dequeuedItem = false;
        do
        {
            CreatorSemaphore_Wait(self->QueueLock, 1);
            if (self->Used > 0)
            {
                result = self->Items[self->Head];
                self->Used = self->Used - 1;
                self->Head = (self->Head + 1) % self->Size;
                dequeuedItem = true;
            }
            CreatorSemaphore_Release(self->QueueLock, 1);
            if (self->WaitForItem && !dequeuedItem)
            {
                CreatorSemaphore_Wait(self->WaitForItem, 1);
            }
        } while (self->WaitForItem && !dequeuedItem);
    }
    return result;
}

void* CreatorQueue_DequeueWaitFor(CreatorQueue self, uint milliseconds)
{
    void* result = NULL;
    if (self)
    {
        bool dequeuedItem = false;
        int count = 0;
        do
        {
            CreatorSemaphore_Wait(self->QueueLock, 1);
            if (self->Used > 0)
            {
                result = self->Items[self->Head];
                self->Used = self->Used - 1;
                self->Head = (self->Head + 1) % self->Size;
                dequeuedItem = true;
            }
            CreatorSemaphore_Release(self->QueueLock, 1);
            if (self->WaitForItem && !dequeuedItem && (count == 0))
            {
                CreatorSemaphore_WaitFor(self->WaitForItem, 1, milliseconds);
            }
            count++;
        } while (self->WaitForItem && !dequeuedItem && (count < 2));
    }
    return result;
}

int CreatorQueue_GetCount(CreatorQueue self)
{
    int result = 0;
    if (self)
    {
        CreatorSemaphore_Wait(self->QueueLock, 1);
        result = self->Used;
        CreatorSemaphore_Release(self->QueueLock, 1);
    }
    return result;
}

void CreatorQueue_Free(CreatorQueue *self)
{
    CreatorQueue queue = *self;
    if (queue)
    {
        if (queue->WaitForItem)
        {
            CreatorSemaphore waitForItem = queue->WaitForItem;
            queue->WaitForItem = NULL;
            CreatorSemaphore_Release(waitForItem, 1);
            CreatorThread_SleepMilliseconds(NULL, 1);
            CreatorSemaphore_Free(&waitForItem);
        }
        CreatorSemaphore_Free(&queue->QueueLock);
        Creator_MemFree((void **)&queue->Items);
        Creator_MemFree((void **)self);
    }
}
