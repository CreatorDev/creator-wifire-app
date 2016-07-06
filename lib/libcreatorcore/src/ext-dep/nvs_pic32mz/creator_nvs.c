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

#ifdef MICROCHIP_PIC32

#include <stdbool.h>
#include "creator/core/creator_nvs.h"
#include "creator/core/creator_debug.h"
#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_threading.h"
#include "driver/nvm/drv_nvm.h"

#define MAX_SEMAPHORE_WAIT_TIME_MS	10
#define APPSTART_OFFSET				0x4DC
//#define NVS_BASE_ADDRESS			0xBD1F0000   //Does work anymore base address is now number of blocks in
#define NVS_BASE_ADDRESS			0

static CreatorSemaphore _NVSLock = NULL;

static DRV_HANDLE _NVMHandle;
static DRV_NVM_COMMAND_HANDLE _NVMBufferHandle;

static uint8 __attribute__((coherent)) _NVSCache[DRV_NVM_PAGE_SIZE];

#define NVS_MAGIC_NUMBER (uint64_t) 0x0101020305080D15

// Creator NV key-value configuration
#define CONFIG_STORE_KEYNAME_LENGTH							(48)
#define CONFIG_STORE_KEYVALUE_LENGTH						(256)		// TODO - max SessionToken len?
#define CONFIG_STORE_KEY_NUMBER								(4)			// number of key value pairs

// Creator NV key-value configuration (keyname is null terminated)
typedef struct
{
    size_t ValueLength;
    char Key[CONFIG_STORE_KEYNAME_LENGTH+1];
    uint8 Value[CONFIG_STORE_KEYVALUE_LENGTH];
}ConfigCreatorKeyValue;

//ConfigCreatorKeyValue	_CreatorKeyValue[CONFIG_STORE_KEY_NUMBER];
ConfigCreatorKeyValue *_CreatorKeyValue = NULL;

bool CreatorNVS_Initialise(void)
{
    bool result = false;
    if (_NVSLock)
    result = true;
    else
    {
        /* Attempt to create a semaphore. */
        _NVSLock = CreatorSemaphore_New(1,1);
        if (_NVSLock)
        {
            _NVMHandle = DRV_NVM_Open(DRV_NVM_INDEX_0, DRV_IO_INTENT_READWRITE);
            if (DRV_HANDLE_INVALID == _NVMHandle)
            {
                SYS_ASSERT(DRV_HANDLE_INVALID != _NVMHandle, "Error during initialisation of ConfigStore");
            }
            else
            {
                bool successfulRead = false;

                DRV_NVM_Read(_NVMHandle, &_NVMBufferHandle, _NVSCache, NVS_BASE_ADDRESS, DRV_NVM_PAGE_SIZE);
                if (_NVMBufferHandle == DRV_NVM_COMMAND_HANDLE_INVALID)
                {
                    SYS_ASSERT(false, "Driver Read Failed");
                    result = false;
                }
                else
                {
                    while (DRV_NVM_CommandStatus(_NVMHandle, _NVMBufferHandle) != DRV_NVM_COMMAND_COMPLETED);

                    successfulRead = *((uint64_t*)_NVSCache) == NVS_MAGIC_NUMBER;
                }
                if (!successfulRead)
                {
                    *((uint64_t*)_NVSCache) = NVS_MAGIC_NUMBER;
                    memset(_NVSCache + 8, 0, DRV_NVM_PAGE_SIZE-8);
                }
                _CreatorKeyValue = (ConfigCreatorKeyValue *)(_NVSCache + 8);

                CreatorSemaphore_Release(_NVSLock,1);
                result = true;
            }
        }
    }
    return result;
}

bool NVS_Read(size_t offset, void *value, size_t size)
{
    bool result = false;
    if (CreatorSemaphore_WaitFor(_NVSLock, 1, MAX_SEMAPHORE_WAIT_TIME_MS))
    {
        memcpy(value, _NVSCache + offset, size);
        result = true;
        CreatorSemaphore_Release(_NVSLock,1);
    }
    return result;
}

bool CreatorNVS_Read(size_t offset, void *value, size_t size)
{
    return NVS_Read(APPSTART_OFFSET + offset, value, size);
}

bool NVS_Write(size_t offset, const void *value, size_t size)
{
    bool result = false;
    if (CreatorSemaphore_WaitFor(_NVSLock, 1, MAX_SEMAPHORE_WAIT_TIME_MS))
    {
        memcpy((void*)_NVSCache + offset, value, size);
        DRV_NVM_Erase(_NVMHandle, &_NVMBufferHandle, NVS_BASE_ADDRESS, 1); //DRV_NVM_PAGE_SIZE

        if (_NVMBufferHandle == DRV_NVM_COMMAND_HANDLE_INVALID)
        {
            result = false;
            SYS_ASSERT(false, "Erase failed when saving configuration");
        }
        else
        {
            while (DRV_NVM_CommandStatus(_NVMHandle, _NVMBufferHandle) != DRV_NVM_COMMAND_COMPLETED);
            DRV_NVM_Write(_NVMHandle, &_NVMBufferHandle, (uint8 *)_NVSCache, NVS_BASE_ADDRESS, DRV_NVM_PAGE_SIZE/DRV_NVM_ROW_SIZE);
            
            if (_NVMBufferHandle == DRV_NVM_COMMAND_HANDLE_INVALID)
            {
                result = false;
                SYS_ASSERT(false, "Write failed when saving configuration");
            }
            else
            {
                while (DRV_NVM_CommandStatus(_NVMHandle, _NVMBufferHandle) != DRV_NVM_COMMAND_COMPLETED);
                result = true;
            }
        }
        CreatorSemaphore_Release(_NVSLock,1);
    }
    return result;
}

bool CreatorNVS_Write(size_t offset, const void *value, size_t size)
{
    return NVS_Write(APPSTART_OFFSET + offset, value, size);
}

void CreatorNVS_SetCache(size_t offset, uint8_t value, size_t size)
{
    memset((void*)_NVSCache + APPSTART_OFFSET + offset, value, size);
}

void *CreatorNVS_GetCacheAddress(size_t offset)
{
    return (void*)_NVSCache + APPSTART_OFFSET + offset;
}

static ConfigCreatorKeyValue *GetCreatorKeyValuePair(const char *keyName)
{
    ConfigCreatorKeyValue *result = NULL;
    if (keyName)
    {
        int i;
        ConfigCreatorKeyValue *keyValuePair;
        for (i = 0; i < CONFIG_STORE_KEY_NUMBER; i++)
        {
            // TODO - case independent compare?
            keyValuePair = &_CreatorKeyValue[i];
            if (*keyValuePair->Key && (strcmp(keyName, keyValuePair->Key) == 0))
            {
                // Keyname found
                if (*keyValuePair->Value)
                result = keyValuePair;
                break;
            }
        }
    }
    return result;
}

bool SetCreatorKeyValue(const char *keyName, const uint8 *value, size_t valueLength)
{
    bool result = false;
    if (value && valueLength > CONFIG_STORE_KEYVALUE_LENGTH)
    return result;		// Ignore value - too long
    ConfigCreatorKeyValue *keyValuePair = GetCreatorKeyValuePair(keyName);
    if (keyValuePair)
    {
        if (value)
        {
            // Store new value (erase old value in case it's longer for security)
            memset(keyValuePair->Value, 0, CONFIG_STORE_KEYVALUE_LENGTH);
            memcpy(keyValuePair->Value, value, valueLength);
            keyValuePair->ValueLength = valueLength;
        }
        else
        {
            // Erase key-value pair
            memset(keyValuePair, 0, sizeof(ConfigCreatorKeyValue));
        }
        result = true;
    }
    else if (value)
    {
        // Key not found, get first free key-value location
        int i;
        ConfigCreatorKeyValue *keyValuePair;
        for (i = 0; i < CONFIG_STORE_KEY_NUMBER; i++)
        {
            keyValuePair = &_CreatorKeyValue[i];
            if (*keyValuePair->Key == 0)
            {
                // Store new key-value
                memset(keyValuePair, 0, sizeof(ConfigCreatorKeyValue));
                strncpy(keyValuePair->Key, keyName, CONFIG_STORE_KEYNAME_LENGTH);
                memcpy(keyValuePair->Value, value, valueLength);
                keyValuePair->ValueLength = valueLength;
                result = true;
                break;
            }
        }
    }
    return result;
}

void CreatorNVS_Set(const char *key, const void *value, size_t size)
{
    bool success = SetCreatorKeyValue(key, value, size);
    NVS_Write(8, _CreatorKeyValue, CONFIG_STORE_KEY_NUMBER * sizeof(ConfigCreatorKeyValue));
}

void *CreatorNVS_Get(const char *key, size_t *size)
{
    void *result = NULL;
    if (size)
    *size = 0;
    ConfigCreatorKeyValue *keyValuePair = GetCreatorKeyValuePair(key);
    if (keyValuePair && keyValuePair->ValueLength > 0)
    {
        result = Creator_MemAlloc(keyValuePair->ValueLength);
        if (result)
        {
            memcpy(result, keyValuePair->Value, keyValuePair->ValueLength);
            if (size)
            *size = keyValuePair->ValueLength;
        }
    }
    return result;
}

void CreatorNVS_SetLocation(const char *location)
{

}

void CreatorNVS_Shutdown(void)
{

}

#endif
