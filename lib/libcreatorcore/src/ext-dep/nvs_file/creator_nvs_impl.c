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

#ifdef POSIX

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "creator/core/creator_list.h"

#include "creator_nvs_file.h"

#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_debug.h"
#include "creator/core/creator_threading.h"


typedef struct
{
    char *Key;
    size_t ValueSize;
    void *Value;
}NVSElement;

static char _NVSFilename[512];
static CreatorList _NVSData = NULL;
static CreatorSemaphore _DataMutex;

static bool ReadDataFromFile(void);
static bool WriteDataToFile(void);

static int FindIndexOfKey(const char *key);

bool CreatorNVSFile_Initialise(const char *filename)
{
    Creator_Assert(filename != NULL, "NVS: path to filename is NULL");
    if (filename == NULL)
    {
        return false;
    }
    //save file path
    size_t pathLength = strlen(filename);
    if (pathLength >= sizeof(_NVSFilename))
    {
        Creator_Assert(filename != NULL, "NVS: path to filename is too long");
        return false;
    }
    strcpy(_NVSFilename, filename);

    //initialize in-memory data copy
    _NVSData = CreatorList_New(10);
    if (_NVSData)
    {
        ReadDataFromFile();
    }

    _DataMutex = CreatorSemaphore_New(1, 0);

    return _NVSData && _DataMutex;
}

void CreatorNVSFile_Set(const char *key, const void *value, size_t size)
{
    if (!key)
    {
        Creator_Log(CreatorLogLevel_Error, "Invalid NULL key to CreatorNVSFile_Set");
        return;
    }

    CreatorSemaphore_Wait(_DataMutex, 1);

    int targetIndex = FindIndexOfKey(key);
    NVSElement *element = NULL;
    if (targetIndex != -1)
    {
        //find item with the same key and replace the value
        element = CreatorList_GetItem(_NVSData, (uint)targetIndex);
    }
    else
    {
        //no existing item with the same key, add it
        size_t keySize = strlen(key) + 1;
        char *keyCopy = Creator_MemAlloc(keySize);
        strcpy(keyCopy, key);

        element = Creator_MemAlloc(sizeof(NVSElement));
        if (element)
        {
            memset(element,0,sizeof(NVSElement));
            //create item then get ref to it
            CreatorList_Add(_NVSData, element);
            element->Key = keyCopy;
        }
    }

    if (element)
    {
        //clear previous value
        if (element->Value)
        {
            Creator_MemFree((void **)&element->Value);
        }

        //copy new value
        element->ValueSize = size;
        if (size && value)
        {
            element->Value = Creator_MemAlloc(size);
            if (element->Value)
            {
                memcpy(element->Value, value, size);
            }
        }

        //commit
        WriteDataToFile();
    }

    CreatorSemaphore_Release(_DataMutex, 1);
}

void *CreatorNVSFile_Get(const char *key, size_t *size)
{
    if (!key)
    {
        Creator_Log(CreatorLogLevel_Error, "Invalid NULL key to CreatorNVSFile_Set");
        return NULL;
    }

    void *returnValue = NULL;

    CreatorSemaphore_Wait(_DataMutex, 1);

    int targetIndex = FindIndexOfKey(key);

    //find item with the same key and replace the value
    if (targetIndex != -1)
    {
        NVSElement *element = CreatorList_GetItem(_NVSData, (uint)targetIndex);

        if (size)
        {
            *size = element->ValueSize;
        }
        if (element->ValueSize)
        {
            returnValue = Creator_MemAlloc(element->ValueSize);
            if (returnValue)
            {
                memcpy(returnValue, element->Value, element->ValueSize);
            }
        }
    }

    CreatorSemaphore_Release(_DataMutex, 1);

    return returnValue;
}

void CreatorNVSFile_Shutdown(void)
{
    uint itemCount = CreatorList_GetCount(_NVSData);
    uint index;

    //find item with the same key and replace the value
    for (index = 0; index < itemCount; index++)
    {
        NVSElement *pElem = CreatorList_GetItem(_NVSData, index);
        Creator_MemFree((void **)&pElem->Key);
        if (pElem->Value)
        {
            Creator_MemFree((void **)&pElem->Value);
        }
    }

    CreatorList_Free(&_NVSData, true);
    if (_DataMutex)
    {
        CreatorSemaphore_Wait(_DataMutex, 1);
        CreatorSemaphore_Release(_DataMutex, 1);
        CreatorSemaphore_Free(&_DataMutex);
    }
}

static bool ReadDataFromFile(void)
{
    int fd = open(_NVSFilename, O_RDONLY);
    if (fd == -1)
    {
        Creator_Log(CreatorLogLevel_Error, "Failed to open file %s in read-only mode", _NVSFilename);
        return false;
    }
    bool fileIsCorrupt = false;
    bool hasMoreToRead = true;

    while (hasMoreToRead)
    {
        NVSElement *newElement = Creator_MemAlloc(sizeof(NVSElement));
        if (!newElement)
        break;
        memset(newElement,0, sizeof(NVSElement));
        ssize_t readBytes = 0;
        bool bItemIsCorrupt = false;

        //read key (size then key)
        size_t keySize = 0;
        readBytes = read(fd, &keySize, sizeof(keySize));
        if (readBytes == 0)
        {
            Creator_MemFree((void **)&newElement);
            hasMoreToRead = false;
            continue;
        }
        fileIsCorrupt |= (readBytes != sizeof(keySize) || keySize <= 0);

        if (!fileIsCorrupt)
        {
            newElement->Key = Creator_MemAlloc(keySize);
            if (newElement->Key)
            {
                readBytes = read(fd, newElement->Key, keySize);
                fileIsCorrupt |= (readBytes != (ssize_t)keySize);
            }
            else
            {
                off_t seekResult = lseek(fd, (off_t)keySize, SEEK_CUR);
                fileIsCorrupt |= (seekResult != (off_t)keySize);
                Creator_Log(CreatorLogLevel_Error, "Failed to allocate %ld bytes for NVS key", keySize);
                bItemIsCorrupt = true;
            }
        }

        //read value (size then data)
        if (!fileIsCorrupt)
        {
            readBytes = read(fd, &newElement->ValueSize, sizeof(newElement->ValueSize));
            fileIsCorrupt |= (readBytes != sizeof(newElement->ValueSize));
        }
        if (!fileIsCorrupt && newElement->ValueSize)
        {
            newElement->Value = Creator_MemAlloc(newElement->ValueSize);
            if (newElement->Value)
            {
                readBytes = read(fd, newElement->Value, newElement->ValueSize);
                fileIsCorrupt |= (readBytes != (ssize_t)newElement->ValueSize);
            }
            else
            {
                off_t seekResult = lseek(fd, (off_t)newElement->ValueSize, SEEK_CUR);
                fileIsCorrupt |= (seekResult != (off_t)newElement->ValueSize);
                Creator_Log(CreatorLogLevel_Error, "Failed to allocate %ld bytes for NVS data", newElement->ValueSize);
                bItemIsCorrupt = true;
            }
        }

        if (!fileIsCorrupt && !bItemIsCorrupt)
        {
            //add element to local copy
            CreatorList_Add(_NVSData, newElement);
        }
        else
        {
            Creator_MemFree((void **)&newElement);
        }

        if (fileIsCorrupt)
        {
            hasMoreToRead = false;
        }
    }

    if (fileIsCorrupt)
    {
        Creator_Log(CreatorLogLevel_Error, "NVS data file is corrupt");
    }

    close(fd);
    return true;
}

static bool WriteDataToFile(void)
{
    int fd = open(_NVSFilename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1)
    {
        Creator_Log(CreatorLogLevel_Error, "Failed to open/create file %s in write-only mode", _NVSFilename);
        return false;
    }
    uint itemCount = CreatorList_GetCount(_NVSData);
    uint index;

    //find item with the same key and replace the value
    for (index = 0; index < itemCount; index++)
    {
        NVSElement *pElem = CreatorList_GetItem(_NVSData, index);
        if (pElem->ValueSize == 0)
        {
            //skip empty elements, to avoid hanging onto keys with no values
            continue;
        }

        size_t keySize = strlen(pElem->Key) + 1;

        ssize_t writtenBytes = 0;
        bool failedWriting = false;
        writtenBytes = write(fd, &keySize, sizeof(keySize));
        failedWriting |= (writtenBytes != sizeof(keySize));
        writtenBytes = write(fd, pElem->Key, keySize);
        failedWriting |= (writtenBytes != (ssize_t)keySize);
        writtenBytes = write(fd, &pElem->ValueSize, sizeof(pElem->ValueSize));
        failedWriting |= (writtenBytes != sizeof(pElem->ValueSize));
        writtenBytes = write(fd, pElem->Value, pElem->ValueSize);
        failedWriting |= (writtenBytes != (ssize_t)pElem->ValueSize);

        if (failedWriting)
        {
            Creator_Log(CreatorLogLevel_Error, "NVS failed to write to data file, file is now corrupt", index);
            break;
        }
    }

    close(fd);
    return true;
}

static int FindIndexOfKey(const char *key)
{
    uint itemCount = CreatorList_GetCount(_NVSData);
    uint index;
    int targetIndex = -1;

    //find item with the same key and replace the value
    for (index = 0; index < itemCount; index++)
    {
        NVSElement *pElem = CreatorList_GetItem(_NVSData, index);
        if (strcmp(pElem->Key, key) == 0)
        {
            targetIndex = index;
            break;
        }
    }

    return targetIndex;
}

#endif
