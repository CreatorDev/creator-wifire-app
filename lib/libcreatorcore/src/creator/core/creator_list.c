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

#include "creator/core/creator_list.h"
#include "creator/core/base_types.h"
#include "creator/core/creator_memalloc.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

struct CreatorListImpl
{
    uint Size;
    uint Used;
    void **Items;
};

CreatorList CreatorList_New(uint initialCapacity)
{
    CreatorList result = NULL;
    result = Creator_MemAlloc(sizeof(struct CreatorListImpl));
    if (result)
    {
        if (initialCapacity == 0)
            initialCapacity = 10;
        result->Used = 0;
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
    return result;
}

bool CreatorList_Add(CreatorList self, void *item)
{
    bool result = false;
    if (self)
    {
        uint count = self->Used + 1;
        if (count > self->Size)
        {
            void **newItems = Creator_MemRealloc(self->Items, (self->Size + 5) * sizeof(void *));
            if (newItems)
            {
                self->Size += 5;
                self->Items = newItems;
            }
        }
        if (count <= self->Size)
        {
            self->Items[count - 1] = item;
            self->Used = count;
            result = true;
        }
    }
    return result;
}

bool CreatorList_Contains(CreatorList self, void *item)
{
    bool result = false;
    if (self)
    {
        uint index;
        for (index = 0; index < self->Used; index++)
        {
            if (self->Items[index] == item)
            {
                result = true;
                break;
            }
        }
    }
    return result;
}

uint CreatorList_GetCount(CreatorList self)
{
    uint result = 0;
    if (self)
    {
        result = self->Used;
    }
    return result;
}

void *CreatorList_GetItem(CreatorList self, uint index)
{
    void *result = NULL;
    if (self)
    {
        if (index < self->Used)
        {
            result = self->Items[index];
        }
    }
    return result;
}

bool CreatorList_Remove(CreatorList self, void *item)
{
    bool result = false;
    if (self)
    {
        uint index;
        for (index = 0; index < self->Used; index++)
        {
            if (self->Items[index] == item)
            {
                result = true;
                break;
            }
        }
        if (result)
        {
            self->Used--;
            uint moveIndex;
            for (moveIndex = index; moveIndex < self->Used; moveIndex++)
            {
                self->Items[moveIndex] = self->Items[moveIndex + 1];
            }
        }
    }
    return result;
}

void *CreatorList_RemoveAt(CreatorList self, uint index)
{
    void *result = NULL;
    if (self)
    {
        if (index < self->Used)
        {
            result = self->Items[index];
            self->Used--;
            uint moveIndex;
            for (moveIndex = index; moveIndex < self->Used; moveIndex++)
            {
                self->Items[moveIndex] = self->Items[moveIndex + 1];
            }
        }
    }
    return result;
}

void CreatorList_Free(CreatorList *self, bool freeItems)
{
    if (self && *self)
    {
        CreatorList list = *self;
        if (freeItems)
        {
            uint index;
            for (index = 0; index < list->Used; index++)
            {
                if (list->Items[index])
                {
                    Creator_MemFree((void **)&list->Items[index]);
                }
            }
        }
        Creator_MemFree((void **)&list->Items);
        Creator_MemFree((void **)self);
    }
}

