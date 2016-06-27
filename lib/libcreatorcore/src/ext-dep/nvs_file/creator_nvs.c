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

#include "creator_nvs_file.h"
#include "creator/core/base_types_methods.h"

#ifndef CREATOR_NVSFILE_FILENAME
#define CREATOR_NVSFILE_FILENAME "libcreator-nvs"
#endif

char *_NVSFilePath = CREATOR_NVSFILE_FILENAME;
char *_AllocatedFilePath = NULL;

void *CreatorNVS_Get(const char *key, size_t *size)
{
    return CreatorNVSFile_Get(key, size);
}

bool CreatorNVS_Initialise(void)
{
    return CreatorNVSFile_Initialise(_NVSFilePath);
}

void CreatorNVS_Set(const char *key, const void *value, size_t size)
{
    CreatorNVSFile_Set(key, value, size);
}

void CreatorNVS_SetLocation(const char *location)
{
    if (_AllocatedFilePath)
    CreatorString_Free(&_AllocatedFilePath);
    _AllocatedFilePath = _NVSFilePath = CreatorString_Duplicate(location);
}

void CreatorNVS_Shutdown(void)
{
    CreatorNVSFile_Shutdown();
    if (_AllocatedFilePath)
    CreatorString_Free(&_NVSFilePath);
}

bool CreatorNVS_Read(size_t offset, void *value, size_t size)
{
    return false;
}

bool CreatorNVS_Write(size_t offset, const void *value, size_t size)
{
    return false;
}

#endif
