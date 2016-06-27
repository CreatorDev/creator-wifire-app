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

#ifndef  _H_CREATOR_SIPCMP_COMON_H_
#define  _H_CREATOR_SIPCMP_COMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "creator/core/creator_debug.h"
#include "creator/core/base_types.h"


#define CREATOR_TCP         1
#define CREATOR_TLS         2
#define CREATOR_UDP         3
#define CREATOR_MAX_CONNECTIONS  3
#define CREATOR_MAX_PACKET_LEN   1500
#define CMP_TASK_STACK_SIZE  0	// Beware - SIP callbacks can send SSL messages!
#define CMP_TASK_PRIORITY  2

//#define COMMON_MESSAGING_DEBUG
#ifdef COMMON_MESSAGING_DEBUG
#define COMMON_MESSAGING_LOG Creator_Log
#else
#define COMMON_MESSAGING_LOG(level, format, ...)   while(0) {}
#endif

typedef enum
{
    CreatorCommonMessaging_CallbackEventType_Response,
    CreatorCommonMessaging_CallbackEventType_Header,
    CreatorCommonMessaging_CallbackEventType_HeaderEnd,
    CreatorCommonMessaging_CallbackEventType_Data,
    CreatorCommonMessaging_CallbackEventType_Finished,
    CreatorCommonMessaging_CallbackEventType_NetworkFailure
} CreatorCommonMessaging_CallbackEventType;

typedef bool (*CreatorCommonMessaging_ProtocolCallBack)(CreatorCommonMessaging_CallbackEventType callbackEvent, char *headerName, char *value, int length, void *context);



typedef struct
{
    uint32 ConnectionSourceAddress;
    uint32 ConnectionDestinationAddress;
    ushort ConnectionSourcePort;
    ushort ConnectionDestinationPort;
    bool IsKeepAliveRequired;
    uchar ConnectionTransportType;
    char *TLSCertificateData;
} CreatorCommonMessaging_ConnectionInformation;

#ifdef __cplusplus
}
#endif

#endif

