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

#ifndef CREATOR_LIST_H_
#define CREATOR_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "creator/core/base_types.h"

/**
 * \class CreatorList
 * Generic List implementation
 */
typedef struct CreatorListImpl *CreatorList;

/**
 * \memberof CreatorList
 * Add item to list.
 *
 * @param self list add to
 * @param item item to add to list
 */
bool CreatorList_Add(CreatorList self, void *item);

/**
 * \memberof CreatorList
 * Check if item is already in list.
 *
 * @param self list to check
 * @param item item to find
 */
bool CreatorList_Contains(CreatorList self, void *item);


void CreatorList_Free(CreatorList *self, bool freeItems);

/**
 * \memberof CreatorList
 * Get number of items in list.
 *
 * @param self list to check
 */
uint CreatorList_GetCount(CreatorList self);

/**
 * \memberof CreatorList
 * Retrieve selected items from list.
 *
 * @param self list to check
 * @param index index of item in list
 */
void *CreatorList_GetItem(CreatorList self, uint index);

/**
 * \memberof CreatorList
 * Create new instance of a list.
 *
 * @param initialCapacity initial number of items that can be added before list will need to grow
 */
CreatorList CreatorList_New(uint initialCapacity);

/**
 * \memberof CreatorList
 * Remove item from list.
 *
 * @param self list to remove item from
 * @param item item to remove from list
 */
bool CreatorList_Remove(CreatorList self, void *item);

/**
 * \memberof CreatorList
 * Remove selected items from list.
 *
 * @param self list to check
 * @param index index of item in list
 */
void *CreatorList_RemoveAt(CreatorList self, uint index);

#ifdef __cplusplus
}
#endif

#endif /* CREATOR_LIST_H_ */
