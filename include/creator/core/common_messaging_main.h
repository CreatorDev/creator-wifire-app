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

#ifndef _COMMON_MESSAGING_MAIN_H_
#define _COMMON_MESSAGING_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "common_messaging_defines.h"
#include "creator/core/base_types.h"

uint32 CreatorCommonMessaging_GetHostByName(const char *hostName);
bool CreatorCommonMessaging_SendRequest(void *connectionInformation, char *sendBuffer, uint16 sendBufferLength, ushort responseTimeout);

void *CreatorCommonMessaging_CreateConnection(CreatorCommonMessaging_ConnectionInformation *connectionInformation, CreatorCommonMessaging_ProtocolCallBack protocolCallBack, void *callbackContext);
void CreatorCommonMessaging_DeleteConnection(void *connectionInformation);

void CreatorCommonMessaging_ChangeConnectionCallBack(void * connectionInformation, CreatorCommonMessaging_ProtocolCallBack protocolCallBack);

void CreatorCommonMessaging_Initialise(void);
void CreatorCommonMessaging_Shutdown(void);

void CreatorCommonMessaging_IPAddressToString(uint address, char *dest);
void CreatorCommonMessaging_LockTCP(void);
void CreatorCommonMessaging_UnLockTCP(void);

#ifdef __cplusplus
}
#endif

#endif
