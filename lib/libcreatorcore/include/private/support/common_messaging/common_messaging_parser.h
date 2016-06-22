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

#ifndef _COMMON_MESSAGING_PARSER_H_
#define _COMMON_MESSAGING_PARSER_H_

#include "creator/core/common_messaging_defines.h"
#include "creator/core/base_types.h"

typedef struct
{
	bool Enabled;
	bool ResponsePending;
	bool IsKeepAliveRequired;
	int32 ConnectionHandle;
	char *ReceivedBuffer;
	char *TemporaryDataBuffer;
	CreatorCommonMessaging_ProtocolCallBack ProtocolCallBack;
	void *CallbackContext;
	ushort PacketOffsetLength; //is used in case of incomplete header..
	bool IsPacketBegining; 
	bool IsContentReadingInProgress;
	ushort LengthOfRemainingContent;
	ushort TemporaryDataBufferLength;
	uchar TransportType;
	uint32 ConnectionDestinationAddress;
	ushort ConnectionDestinationPort;
	uint32 ResponseTimeout;				// max response time (in SYS_TICKS)
	uint32 SendStartTime;				// send start time (SYS_TICKS)
	unsigned char *TLSCertificateData;
	int TLSCertificateSize;
	void *SSLSession;
} CreatorCommonMessaging_ControlBlock;

void CreatorCommonMessaging_Receive(CreatorCommonMessaging_ControlBlock *controlBlock);
int32 CreatorCommonMessaging_HandleContent(CreatorCommonMessaging_ControlBlock *controlBlock, char *contentBuffer, int *length);
int CreatorCommonMessaging_ParseMessage(CreatorCommonMessaging_ControlBlock *controlBlock, char *dataBuffer, int bufferLength);

#endif
