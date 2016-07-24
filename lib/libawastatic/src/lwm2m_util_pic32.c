/************************************************************************************************************************
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
************************************************************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <inttypes.h>

#include "creator/core/creator_timer.h"
#include "lwm2m_util.h"
#include "lwm2m_list.h"
#include "lwm2m_debug.h"


uint64_t Lwm2mCore_GetTickCountMs(void)
{
    return CreatorTimer_GetTickCount();
}

void Lwm2mCore_AddressTypeToPath(char * path, size_t pathSize, AddressType * addr)
{
    char buffer[20];
    
    memcpy(path, "coap", 4);
    path += 4;
    pathSize -= 4;

    if (addr->Secure)
    {
        *path = 's';
        path++;
        pathSize--;
    }
    
    // TODO - fixme for DTLS (use network abstraction: get uri or secure for protocol)
    sprintf(buffer, "%d.%d.%d.%d", (uint8_t)addr->Address, (uint8_t)(addr->Address >> 8), (uint8_t)(addr->Address >> 16), (uint8_t)(addr->Address >> 24));
    //port = ntohs(addr->Port);
    snprintf(path, pathSize, "://%s:%d", &buffer, addr->Port);
}

const char * Lwm2mCore_DebugPrintAddress(AddressType * addr)
{
    static char result[60];
    char buffer[20];

    sprintf(buffer, "%d.%d.%d.%d", (uint8_t)addr->Address, (uint8_t)(addr->Address >> 8), (uint8_t)(addr->Address >> 16), (uint8_t)(addr->Address >> 24));
    snprintf(result, sizeof(result), "%s:%d", &buffer, addr->Port);

    return (const char *)&result;
}

bool Lwm2mCore_ResolveAddressByName(unsigned char * address, int addressLength, AddressType * addr)
{
    /* Not used by Erbium client */
    bool result = false;
    return result;
}

int Lwm2mCore_CompareAddresses(AddressType * addr1, AddressType * addr2)
{
    int result = addr1->Address - addr2->Address;
    return result;
}

int Lwm2mCore_ComparePorts(AddressType * addr1, AddressType * addr2)
{
    int result = addr1->Port - addr2->Port;
    return result;
}

int Lwm2mCore_GetIPAddressFromInterface(const char * interface, int addressFamily, char * destAddress, size_t destAddressLength)
{
    /* Note: only used by servers */
    int returnCode = 0;
    return returnCode;
}

