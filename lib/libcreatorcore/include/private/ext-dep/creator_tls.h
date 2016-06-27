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

#ifndef CREATOR_TLS_H_
#define CREATOR_TLS_H_

#include <stdbool.h>
#include "common_messaging_parser.h"

typedef enum
{
    CreatorTLSError_NotSet = 0,
    CreatorTLSError_None,
    CreatorTLSError_RecieveBufferEmpty,     //an error occurred, but no-one knows what it was
    CreatorTLSError_TransmitBufferFull,
    CreatorTLSError_ConnectionClosed,
    CreatorTLSError_ConnectionReset
} CreatorTLSError;

void CreatorTLS_Initialise(void);
void CreatorTLS_Shutdown(void);


bool CreatorTLS_StartSSLSession(CreatorCommonMessaging_ControlBlock *controlBlock, bool client);
void CreatorTLS_EndSSLSession(CreatorCommonMessaging_ControlBlock *controlBlock);

int CreatorTLS_Read(CreatorCommonMessaging_ControlBlock *controlBlock, void *buffer, size_t bufferSize);
int CreatorTLS_Write(CreatorCommonMessaging_ControlBlock *controlBlock,void *buffer, size_t length);

CreatorTLSError CreatorTLS_GetError(CreatorCommonMessaging_ControlBlock *controlBlock);

#endif /* CREATOR_TLS_H_ */
