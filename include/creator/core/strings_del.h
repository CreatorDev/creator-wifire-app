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

#ifndef CREATOR_CORE_STRINGS_H_
#define CREATOR_CORE_STRINGS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "creator/core/base_types.h"
#include "creator/core/creator_memorymanager.h"
#include "creator/core/creator_memorymanager_methods.h"
#include "creator/core/creator_object.h"
#include "creator/core/creator_object_methods.h"
#include "creator/core/strings_type.h"
#include "creator/core/xml_serialisation.h"

/**
 * \memberof CreatorStrings
 * \brief Creates a new CreatorStrings.
 * 
 * You should use the result of this method whenever you need to submit a Object as a \a data parameter.
 * \param memoryManager Memory manager on which the method will be applied
*/
static inline CreatorStrings CreatorStrings_New(CreatorMemoryManager memoryManager){ return (CreatorStrings)CreatorMemoryManager_NewObject(memoryManager, 1, 0, CreatorType_Strings);}

/**
 * \memberof CreatorStrings
 * \brief Registers meta data used for (de)serialising CreatorStrings to XML.
 * 
 * You should this if you are going to use this type in application code.
*/
static inline void  CreatorStrings_RegisterType()
{
	if (!CreatorXMLDeserialiser_IsRegisteredType(CreatorType_Strings))
	{
		CreatorXMLDeserialiser_RegisterType(CreatorType_Strings, "Strings", 1, 0);
		CreatorXMLDeserialiser_RegisterTypeArrayProperty(CreatorType_Strings, _CreatorStrings_Items, "Items", "Item", CreatorType_String, CreatorType_Strings);
	}
}

/**
 * \memberof CreatorStrings
 * \brief Indicates whether the property Items is set on this object.
 * 
 * Can be used to check whether Items is available on this object without making an HTTP call.
 * \param self Object on which the method will be applied
*/

static inline bool CreatorStrings_HasItems(CreatorStrings self) { return CreatorObject_HasProperty(self, _CreatorStrings_Items);}
/**
 * \memberof CreatorStrings
 * \brief Gets the item at a given position in the list.
 * 
 * \param self Object on which the method will be applied
 * \param index index of item in collection
*/

static inline char *CreatorStrings_GetItem(CreatorStrings self, uint index) { return CreatorObject_GetArrayStringProperty(self, _CreatorStrings_Items, index);}
/**
 * \memberof CreatorStrings
 * \brief Returns the number of items in the list.
 * 
 * The value returned by this method is related to the number of items already loaded in the list.
 * \param self Object on which the method will be applied
*/

static inline int CreatorStrings_GetCount(CreatorStrings self) { return CreatorObject_GetArrayPropertyCount(self, _CreatorStrings_Items);}
/**
 * \memberof CreatorStrings
 * \brief Append a new item at the end of the list and returns it.
 * 
 * This is meant to be used to build up lists of items to submit.
 * \param self Object on which the method will be applied
 * \param item to add to collection
*/

static inline void CreatorStrings_NewItem(CreatorStrings self, char * item)
{
    CreatorObject_AddArrayStringProperty(self, _CreatorStrings_Items, item);
}
#ifdef __cplusplus
}
#endif

#endif /* CREATOR_CORE_STRINGS_H_ */
