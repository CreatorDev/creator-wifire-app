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


#ifndef CREATOR_NVS_H_
#define CREATOR_NVS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


/**
 * \brief Initializes the Non Volatile Storage library.
 *
 * Called by \ref CreatorCore_Initialise.
 *
 * @return true if successful, false otherwise
 */
bool CreatorNVS_Initialise(void);

/**
 * \brief Sets a value in Non-Volatile-Storage.
 *
 * if \a value is NULL or \a size is 0, the value should be removed from NVS.
 *
 * @param key key that the value is mapping to
 * @param value binary buffer holding the value to store
 * @param size size of the buffer
 */
void CreatorNVS_Set(const char *key, const void *value, size_t size);

/**
 * \brief Returns a newly allocated buffer containing the value for this key.
 *
 * The buffer should be allocated by \ref Creator_MemAlloc and freed by the caller using \ref Creator_MemFree.
 *
 * @param key key that the value is mapping to
 * @param[out] size pointer to the size of buffer that is returned
 * @return NULL if data could not be allocated or found for that key.
 */
void *CreatorNVS_Get(const char *key, size_t *size);

/**
 * \brief Read non-volatile value.
 *
 * Read current value from cached non-volatile value.
 *
 * @param offset Offset into NV storage
 * @param value Value set by read
 * @param size Size to read
 * @return true if data was read
 */
bool CreatorNVS_Read(size_t offset, void *value, size_t size);

/**
 * \brief Write non-volatile value.
 *
 * Write current value to non-volatile storage (updates cached values)
 *
 * @param offset Offset into NV storage
 * @param value Value stored by write
 * @param size Size to write
 * @return true if data was stored
 */
bool CreatorNVS_Write(size_t offset, const void *value, size_t size);

/**
 * \brief Set cached data.
 *
 * Fill cached data with specified value. Note: cached data is stored on the next Write().
 *
 * @param offset Offset into NV storage
 * @param value Value to fill cache
 * @param size Size to fill
 */
void CreatorNVS_SetCache(size_t offset, uint8_t value, size_t size);

/**
 * \brief Get cache address.
 *
 * Returns address of current cached value. This is intended for use with large values stored in NV memory without needing to copy the data.
 *
 * @param offset Offset into NV storage
 * @return 
 */
void *CreatorNVS_GetCacheAddress(size_t offset);

void CreatorNVS_SetLocation(const char *location);

/**
 * \brief Frees the Non Volatile Storage library.
 *
 * Called by \ref CreatorCore_Shutdown.
 */
void CreatorNVS_Shutdown(void);


#ifdef __cplusplus
}
#endif

#endif /* CREATOR_NVS_H_ */
