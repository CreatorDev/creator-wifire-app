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

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include "creator/core/common_messaging_defines.h"
#include "creator/core/common_messaging_main.h"
#include "common_messaging_parser.h"
#include "creator_tls.h"
#include "creator/core/creator_threading.h"
#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_timer.h"
#include "creator/core/creator_debug.h"
#include "creator/creator_console.h"

#ifdef MICROCHIP_PIC32
	#include <tcpip/berkeley_api.h>
	#include <sys/errno.h>
#else
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/socket.h>
	#include <netdb.h>
	typedef int SOCKET;
	#define SOCKET_ERROR            (-1)
	#define closesocket close
	#include <netinet/in.h>
	#include <linux/tcp.h>
	#include <linux/version.h>
#endif


//#define CMP_DEBUG_ERR
#ifdef CMP_DEBUG_ERR
	#include "tcpip/src/system/system_debug.h"
#endif

#define CMP_THREAD_SAFE		// Use single thread for tcp + common messaging task
#ifdef CMP_THREAD_SAFE

static CreatorSemaphore _TCPStackLock = NULL;

#endif

static CreatorThread _ProcessingThread = NULL;

static CreatorCommonMessaging_ControlBlock _CommonMessagingControlBlock[CREATOR_MAX_CONNECTIONS];
static bool _IsCommonMessagingTaskCreated = false;
static CreatorSemaphore _DNSRequestLock = NULL;
static CreatorSemaphore _ConnectionLock = NULL;
static bool _Shutdown;

#define CMP_EACH_ITERATION_TASK_WAIT_TIME	1
#define WAIT_TIMEOUT_SECS					60	// wait timeout in seconds


static void CreatorCommonMessaging_Task(CreatorThread thread, void *taskParameters);

#ifndef MICROCHIP_PIC32
#define CREATOR_TCP_KEEPALIVE_PROBES	3
#define CREATOR_TCP_KEEPALIVE_TIME		30
#define CREATOR_TCP_KEEPALIVE_INTERVAL	30
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
#define CREATOR_TCP_MAX_RETRY_TIMEOUT	120000 //120 secs
#endif
#endif

#define  TCP_ENABLE_CLIENT_KEEP_ALIVE   1

void CreatorCommonMessaging_SetKeepAliveParameters(SOCKET socketDescriptor)
{
#ifndef MICROCHIP_PIC32
	int noOfKeepAliveProbes = CREATOR_TCP_KEEPALIVE_PROBES;
	int keepAliveTime = CREATOR_TCP_KEEPALIVE_TIME;
	int keepAliveInterval = CREATOR_TCP_KEEPALIVE_INTERVAL;
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
	unsigned int tcpMaxRetryTimeout = CREATOR_TCP_MAX_RETRY_TIMEOUT;
	#endif
	//creates a variable for KEEPALIVE's optval parm
 	int keepAliveValue;
	//creates a variable for KEEPALIVE's optlen parm
   	socklen_t keepAliveValueLength = sizeof(keepAliveValue);
	// sets KEEPALIVE parms
	keepAliveValue = TCP_ENABLE_CLIENT_KEEP_ALIVE;
	keepAliveValueLength = sizeof(keepAliveValue);
	// turns on KEEPALIVE property on socket
	if (setsockopt (socketDescriptor, SOL_SOCKET, SO_KEEPALIVE, &keepAliveValue, keepAliveValueLength) < 0)
	{
		COMMON_MESSAGING_LOG(CreatorLogLevel_Info, "Enabling KeepAlive Failed");
	}
	setsockopt(socketDescriptor, IPPROTO_TCP, TCP_KEEPCNT, &noOfKeepAliveProbes, sizeof(int));
	setsockopt(socketDescriptor, IPPROTO_TCP, TCP_KEEPIDLE, &keepAliveTime, sizeof(int));
	setsockopt(socketDescriptor, IPPROTO_TCP, TCP_KEEPINTVL, &keepAliveInterval, sizeof(int));
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
		setsockopt(socketDescriptor, IPPROTO_TCP, TCP_USER_TIMEOUT , &tcpMaxRetryTimeout, sizeof(unsigned int));
	#endif
#else
	//creates a variable for KEEPALIVE's optval parm
 	int keepAliveValue;
	//creates a variable for KEEPALIVE's optlen parm
   	int keepAliveValueLength = sizeof(keepAliveValue);
	// sets KEEPALIVE parms
	keepAliveValue = TCP_ENABLE_CLIENT_KEEP_ALIVE;
	keepAliveValueLength = sizeof(keepAliveValue);
	// turns on KEEPALIVE property on socket

	if (setsockopt (socketDescriptor, SOL_SOCKET, SO_KEEPALIVE, (const uint8_t *)&keepAliveValue, keepAliveValueLength) < 0)
	{
		COMMON_MESSAGING_LOG(CreatorLogLevel_Info, "Enabling KeepAlive Failed");
	}

#endif
}

void *CreatorCommonMessaging_CreateConnection(CreatorCommonMessaging_ConnectionInformation *connectionInformation, CreatorCommonMessaging_ProtocolCallBack protocolCallBack, void *callbackContext)
{
	void *result = NULL;
	int lastError;
	int connectStatus;
	int controlBlockCount;
	CreatorCommonMessaging_ControlBlock *controlBlock;
	struct sockaddr_in serverAddress;
	SOCKET clientSocket;
	size_t serverAddressLength =  sizeof(struct sockaddr_in);

	bool connectionError = false;
	/* Create CMP Task */
	/* Check if the task is already created..if not created start the task */

	if (_ConnectionLock)
	{
		// Note: use mutex to make controlBlock + TCP socket allocation thread safe
		CreatorSemaphore_Wait(_ConnectionLock, 1);

		// Get free control block
		COMMON_MESSAGING_LOG(CreatorLogLevel_Info, "Create TCP Connection, dest port=%d", connectionInformation->ConnectionDestinationPort);
		for (controlBlockCount = 0; controlBlockCount < CREATOR_MAX_CONNECTIONS; controlBlockCount++)
		{
			controlBlock = &_CommonMessagingControlBlock[controlBlockCount];
			if (controlBlock->ConnectionHandle == SOCKET_ERROR)
			{
				controlBlock->ProtocolCallBack = protocolCallBack;
				controlBlock->CallbackContext = callbackContext;
				controlBlock->TransportType = connectionInformation->ConnectionTransportType;
				controlBlock->Enabled = false;
				controlBlock->ResponsePending = false;
				controlBlock->IsKeepAliveRequired = connectionInformation->IsKeepAliveRequired;

				controlBlock->TLSCertificateData = (uchar *)connectionInformation->TLSCertificateData;
				if (connectionInformation->TLSCertificateData)
					controlBlock->TLSCertificateSize = strlen(connectionInformation->TLSCertificateData);
				else
					controlBlock->TLSCertificateSize = 0;
				controlBlock->SSLSession = NULL;
				break;
			}
		}
		if (controlBlockCount == CREATOR_MAX_CONNECTIONS)
			controlBlock = NULL; // request failed (BEWARE: don't return within mutex)


		if (controlBlock != NULL)
		{
			CreatorCommonMessaging_LockTCP();
			clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (clientSocket == SOCKET_ERROR)
			{
				connectionError = true;
			}
			else
			{
				memset(&serverAddress,0,serverAddressLength);
				serverAddress.sin_family = AF_INET;
#ifdef MICROCHIP_PIC32
				serverAddress.sin_addr.S_un.S_addr = connectionInformation->ConnectionDestinationAddress;
				serverAddress.sin_port  = connectionInformation->ConnectionDestinationPort;
#else
				serverAddress.sin_addr.s_addr = connectionInformation->ConnectionDestinationAddress;

				int flag = fcntl(clientSocket, F_GETFL);
				flag = flag | O_NONBLOCK;
				if(fcntl(clientSocket, F_SETFL, flag) < 0)
				{

				}

				serverAddress.sin_port  = htons(connectionInformation->ConnectionDestinationPort);
#endif
				controlBlock->ConnectionHandle = clientSocket;
				errno = 0;
				connectStatus = connect( clientSocket,(struct sockaddr *) &serverAddress, serverAddressLength);
				lastError =	errno;
				if(controlBlock->IsKeepAliveRequired)
					CreatorCommonMessaging_SetKeepAliveParameters(clientSocket);	
	//			controlBlock->ConnectionHandle = TCPIP_TCP_ClientOpen(IP_ADDRESS_TYPE_IPV4, connectionInformation->ConnectionDestinationPort,
	//					(IP_MULTI_ADDRESS*) &destinationAddress);
				controlBlock->ConnectionDestinationPort = connectionInformation->ConnectionDestinationPort;
				controlBlock->ConnectionDestinationAddress = connectionInformation->ConnectionDestinationAddress;
			}
			CreatorCommonMessaging_UnLockTCP();
		}

		CreatorSemaphore_Release(_ConnectionLock, 1);
		if (controlBlock == NULL || controlBlock->ConnectionHandle == SOCKET_ERROR)
		{
			Creator_Log(CreatorLogLevel_Error, "Connection failed - no free TCP connection");
		}
		else
		{
			if (!connectionError)
			{
				int timeOutPeriod = CreatorTimer_GetTicksPerSecond() * 20;
				uint startTick = CreatorTimer_GetTickCount();
				while ((connectStatus == SOCKET_ERROR) && ((lastError == EINPROGRESS) || (lastError == EALREADY)) && ((CreatorTimer_GetTickCount()- startTick) < timeOutPeriod))
				{
					CreatorThread_SleepMilliseconds(NULL, 20 );
					CreatorCommonMessaging_LockTCP();
					errno = 0;
					connectStatus = connect( clientSocket,(struct sockaddr *) &serverAddress,serverAddressLength);
					lastError =	errno;
					CreatorCommonMessaging_UnLockTCP();
				}
				connectionError = (connectStatus == SOCKET_ERROR);
			}

			if (connectionError)
			{
				CreatorCommonMessaging_DeleteConnection(controlBlock);
				Creator_Log(CreatorLogLevel_Error, "TCP connection failed - timeout, dest port=%d", connectionInformation->ConnectionDestinationPort);
			}
			else
			{
				if (controlBlock->TransportType == CREATOR_TLS)
				{
					if (!CreatorTLS_StartSSLSession(controlBlock, true))
						connectionError = true;
				}

				if (connectionError)
				{
					CreatorCommonMessaging_DeleteConnection(controlBlock);
				}
				else
				{
					// Get the ephmeral port of the connected socket
					struct sockaddr_in localAddress;
					socklen_t localAddressLength = sizeof(struct sockaddr_in);
					if (getsockname(clientSocket,  (struct sockaddr *)&localAddress, &localAddressLength) == 0)
					{
						connectionInformation->ConnectionSourcePort = ntohs(localAddress.sin_port);
#ifdef MICROCHIP_PIC32
						connectionInformation->ConnectionSourceAddress = localAddress.sin_addr.S_un.S_addr;
#else
						connectionInformation->ConnectionSourceAddress = localAddress.sin_addr.s_addr;
#endif
						controlBlock->Enabled = true;
						COMMON_MESSAGING_LOG(CreatorLogLevel_Info, "TCP Connected: Dest port=%d, source=%d", connectionInformation->ConnectionDestinationPort,	connectionInformation->ConnectionSourcePort);
					}
					result = controlBlock;
				}
			}
		}
	}
	return result;

}


void CreatorCommonMessaging_ChangeConnectionCallBack(void *connectionInformation, CreatorCommonMessaging_ProtocolCallBack protocolCallBack)
{
	if (connectionInformation)
	{
		CreatorSemaphore_Wait(_ConnectionLock, 1);
		CreatorCommonMessaging_ControlBlock *controlBlock = (CreatorCommonMessaging_ControlBlock*) connectionInformation;
		controlBlock->ProtocolCallBack = protocolCallBack;
		CreatorSemaphore_Release(_ConnectionLock, 1);
	}
}

void CreatorCommonMessaging_DeleteConnection(void * connectionInformation)
{
	CreatorCommonMessaging_ControlBlock *controlBlock = (CreatorCommonMessaging_ControlBlock*)connectionInformation;
	if (controlBlock)
	{
		CreatorSemaphore_Wait(_ConnectionLock, 1);
		CreatorCommonMessaging_LockTCP();
		if (controlBlock->ConnectionHandle != SOCKET_ERROR)
		{
			controlBlock->Enabled = false;
			controlBlock->ResponsePending = false;
			closesocket(controlBlock->ConnectionHandle);
			CreatorTLS_EndSSLSession(controlBlock);
			controlBlock->ConnectionHandle = SOCKET_ERROR;
		}
		CreatorCommonMessaging_UnLockTCP();
		CreatorSemaphore_Release(_ConnectionLock, 1);
	}
}

uint32 CreatorCommonMessaging_GetHostByName(const char *hostName)
{
	struct hostent *resolvedAddress = NULL;
	int lastError;
	uint32 result = 0;
	bool isDnsErrorSet = false;

	if (_DNSRequestLock != NULL)
	{
		// Note: use mutex to make DNS requests thread safe, and let concurrent requests succeed (beware SNTP also uses but DNS without mutex protection!)
		CreatorSemaphore_Wait(_DNSRequestLock, 1);

		COMMON_MESSAGING_LOG(CreatorLogLevel_Info, "DNS Gethostbyname %s", hostName);
		int timeOutPeriod = CreatorTimer_GetTicksPerSecond() * WAIT_TIMEOUT_SECS;
		uint startTick = CreatorTimer_GetTickCount();

		if (!isDnsErrorSet)
		{
			CreatorCommonMessaging_LockTCP();
			h_errno = 0;
			resolvedAddress = gethostbyname((char *)hostName);
			lastError =	h_errno;
			CreatorCommonMessaging_UnLockTCP();
			while (!resolvedAddress && (lastError == TRY_AGAIN))
			{
				if ((CreatorTimer_GetTickCount() - startTick) >= timeOutPeriod)
				{
					isDnsErrorSet=true;
					break;
				}
				CreatorThread_SleepMilliseconds(NULL, 20);

				CreatorCommonMessaging_LockTCP();
				h_errno = 0;
				resolvedAddress = gethostbyname((char *)hostName);
				lastError =	h_errno;
				CreatorCommonMessaging_UnLockTCP();
			}
		}
		CreatorSemaphore_Release(_DNSRequestLock, 1);
	}
	if (resolvedAddress)
	{
		COMMON_MESSAGING_LOG(CreatorLogLevel_Info, "DNS Gethostbyname %s done", hostName);
		if (resolvedAddress->h_addrtype == AF_INET)
		{
			struct sockaddr_in serverAddress;
			memcpy(&serverAddress.sin_addr,*(resolvedAddress->h_addr_list),sizeof(serverAddress.sin_addr));
			result = serverAddress.sin_addr.s_addr;
		}
	}
	else
	{
		Creator_Log(CreatorLogLevel_Error, "DNS request host '%s' - %s", hostName, isDnsErrorSet ? "timeout" : "failed");
	}

	return result;
}

void CreatorCommonMessaging_Initialise(void)
{
	_Shutdown = false;
	if (_IsCommonMessagingTaskCreated)
		return;

	int controlBlockCount;
	CreatorCommonMessaging_ControlBlock *controlBlock;
#ifdef CMP_THREAD_SAFE
	if (!_TCPStackLock)
		_TCPStackLock = CreatorSemaphore_New(1, 0);
#endif
	if (!_DNSRequestLock)
		_DNSRequestLock = CreatorSemaphore_New(1, 0);
	if (!_ConnectionLock)
		_ConnectionLock = CreatorSemaphore_New(1, 0);

	for (controlBlockCount = 0; controlBlockCount < CREATOR_MAX_CONNECTIONS; controlBlockCount++)
	{
		controlBlock = &_CommonMessagingControlBlock[controlBlockCount];
		controlBlock->Enabled = false;
		controlBlock->ConnectionHandle = SOCKET_ERROR;
		controlBlock->ReceivedBuffer = Creator_MemAlloc(CREATOR_MAX_PACKET_LEN);
		controlBlock->IsPacketBegining = true;
		controlBlock->PacketOffsetLength = 0;
		controlBlock->TemporaryDataBuffer = NULL; 
		controlBlock->TemporaryDataBufferLength = 0; 
		controlBlock->SSLSession = NULL;
	}

	CreatorTLS_Initialise();

	// Note: must be seperate from the TCP task because SIP receive callbacks can send a message
	// and we mustn't block the TCP task while waiting for room in the Tx window.
	// If SIP receive callbacks didn't send a message within the same task this would make TCP thread safety a lot cleaner.
	if (!_ProcessingThread)
		_ProcessingThread = CreatorThread_New("CMPTask", CMP_TASK_PRIORITY, CMP_TASK_STACK_SIZE, CreatorCommonMessaging_Task, (void *) NULL);

	_IsCommonMessagingTaskCreated = (_ProcessingThread != NULL);
}

void CreatorCommonMessaging_IPAddressToString(uint32 address ,char *dest)
{
	if (dest)
	{
		int b1 = (address >> 24) & 0xFF;
		int b2 = (address >> 16) & 0xFF;
		int b3 = (address >> 8) & 0xFF;
		int b4 = address & 0xFF;
		sprintf(dest, "%d.%d.%d.%d", b1, b2, b3, b4);
	}
}

void CreatorCommonMessaging_LockTCP(void)
{
#ifdef CMP_THREAD_SAFE
	if (!_TCPStackLock)
		_TCPStackLock = CreatorSemaphore_New(1, 0);
	CreatorSemaphore_Wait(_TCPStackLock,1);
#endif
}

bool CreatorCommonMessaging_SendRequest(void *connectionInformation, char *sendBuffer, uint16 sendBufferLength, ushort responseTimeout)
{
	bool result = false;
	CreatorCommonMessaging_ControlBlock *controlBlock = (CreatorCommonMessaging_ControlBlock *) connectionInformation;
	if (controlBlock && controlBlock->Enabled)
	{
		int sentBytes = 0;
		bool sendError = false;
		char *currentBufferLocation = sendBuffer;
		COMMON_MESSAGING_LOG(CreatorLogLevel_Debug, "< SEND(%d) len=%d", controlBlock->ConnectionHandle, sendBufferLength);
		controlBlock->ResponsePending = false;
		controlBlock->ResponseTimeout = 0;										// Don't set timeout until message has been sent.
		if (responseTimeout > 0)
		{
			controlBlock->ResponsePending = true;
		}
		if (controlBlock->TransportType == CREATOR_TLS)
		{
			while (sendBufferLength > 0)
			{
				CreatorTLSError error;
				int timeOutPeriod = CreatorTimer_GetTicksPerSecond() * WAIT_TIMEOUT_SECS;
				uint startTick = CreatorTimer_GetTickCount();
				while (1)
				{
					CreatorCommonMessaging_LockTCP();
					sentBytes = CreatorTLS_Write(controlBlock, currentBufferLocation, sendBufferLength);
					CreatorCommonMessaging_UnLockTCP();
					if (sentBytes < 0)
					{
						error = CreatorTLS_GetError(controlBlock);
						if (error == CreatorTLSError_RecieveBufferEmpty || error == CreatorTLSError_TransmitBufferFull)
						{
							if ((CreatorTimer_GetTickCount() - startTick) >= timeOutPeriod)
							{
								sendError=true;
								break;
							}
							CreatorThread_SleepMilliseconds(NULL, 5);
							continue;
						}
						sendError = true;
						break;
					}
					else if (sentBytes == 0)
					{
						sendError = true;
					}
					break;
				}
				if (sendError)
					break;
				currentBufferLocation += sentBytes;
				sendBufferLength -= sentBytes;
			}
		}
		else
		{
			SOCKET socket = controlBlock->ConnectionHandle;
			int timeOutPeriod = CreatorTimer_GetTicksPerSecond() * WAIT_TIMEOUT_SECS;
			uint startTick = CreatorTimer_GetTickCount();
			while (sendBufferLength > 0)
			{
				CreatorCommonMessaging_LockTCP();
				errno = 0;
				sentBytes = send(socket, currentBufferLocation, sendBufferLength, 0);
				CreatorCommonMessaging_UnLockTCP();
				int lastError = errno;
				if (sentBytes == SOCKET_ERROR)
				{
					if (lastError == EWOULDBLOCK)
					{
						if ((CreatorTimer_GetTickCount() - startTick) >= timeOutPeriod)
						{
							sendError = true;
						}
						else
						{
							CreatorThread_SleepMilliseconds(NULL,5);
							sentBytes = 0;
						}
					}
					else if (lastError == ENOTCONN)
						sendError = true;
					else if (lastError == ECONNRESET)
						sendError = true;
					else if (lastError == EBADF)
						sendError = true;
				}
				else
					startTick = CreatorTimer_GetTickCount();

				if (sendError)
					break;
				currentBufferLocation += sentBytes;
				sendBufferLength -= sentBytes;
			}
		}
		if (sendError)
		{
			controlBlock->ResponsePending = false;
			Creator_Log(CreatorLogLevel_Error, "TCP send failed - dest port=%d", controlBlock->ConnectionDestinationPort);
		}
		else
		{
			if (controlBlock->ResponsePending)
			{
				controlBlock->SendStartTime = CreatorTimer_GetTickCount();
				controlBlock->ResponseTimeout = responseTimeout * CreatorTimer_GetTicksPerSecond();
			}
			COMMON_MESSAGING_LOG(CreatorLogLevel_Debug, "\tTCP send done - dest port=%d", controlBlock->ConnectionDestinationPort);
			result = true;
		}
	}
	return result;
}

void CreatorCommonMessaging_Shutdown(void)
{
	if (!_IsCommonMessagingTaskCreated)
		return;

	_Shutdown = true;

	int controlBlockCount;
	CreatorCommonMessaging_ControlBlock *controlBlock;
	for (controlBlockCount = 0; controlBlockCount < CREATOR_MAX_CONNECTIONS; controlBlockCount++)
	{
		controlBlock = &_CommonMessagingControlBlock[controlBlockCount];
		controlBlock->Enabled = false;
		if (controlBlock->ConnectionHandle != SOCKET_ERROR)
			CreatorCommonMessaging_DeleteConnection(controlBlock);
		if (controlBlock->ReceivedBuffer)
			Creator_MemFree((void **)&controlBlock->ReceivedBuffer);
		controlBlock->IsPacketBegining = true;
		controlBlock->PacketOffsetLength = 0;
		if (controlBlock->TemporaryDataBuffer)
			Creator_MemFree((void **)&controlBlock->TemporaryDataBuffer);
		controlBlock->TemporaryDataBufferLength = 0;
	}
	if (_ProcessingThread)
		CreatorThread_Free(&_ProcessingThread);
	if (_DNSRequestLock)
		CreatorSemaphore_Free(&_DNSRequestLock);
	if (_ConnectionLock)
		CreatorSemaphore_Free(&_ConnectionLock);

#ifdef CMP_THREAD_SAFE
	if (_TCPStackLock)
		CreatorSemaphore_Free(&_TCPStackLock);
#endif

	CreatorTLS_Shutdown();

	_IsCommonMessagingTaskCreated = false;

}


void CreatorCommonMessaging_Receive(CreatorCommonMessaging_ControlBlock *controlBlock)
{
	int receivedDataLength;
	ushort maximumLengthToRead;
	char *receivedBuffer;
	bool connectionLost = false;

	CreatorCommonMessaging_LockTCP();
	if (!controlBlock->Enabled || controlBlock->ConnectionHandle == SOCKET_ERROR)
	{
		CreatorCommonMessaging_UnLockTCP();
		return;
	}
	if (controlBlock->SSLSession == NULL && controlBlock->TransportType == CREATOR_TLS)
	{
		CreatorCommonMessaging_UnLockTCP();
		return;
	}
	if(controlBlock->TemporaryDataBuffer != NULL)
	{
		maximumLengthToRead = controlBlock->TemporaryDataBufferLength - controlBlock->PacketOffsetLength;
		receivedBuffer = controlBlock->TemporaryDataBuffer;
	}
	else
	{
		maximumLengthToRead = CREATOR_MAX_PACKET_LEN - controlBlock->PacketOffsetLength;
		receivedBuffer = controlBlock->ReceivedBuffer;
	}
	receivedDataLength = -1;
	if (controlBlock->TransportType == CREATOR_TLS)
	{
		receivedDataLength = CreatorTLS_Read(controlBlock, receivedBuffer + controlBlock->PacketOffsetLength, maximumLengthToRead);
		if (receivedDataLength < 0)
		{
			CreatorTLSError error = CreatorTLS_GetError(controlBlock);
			if (error != CreatorTLSError_RecieveBufferEmpty && error != CreatorTLSError_TransmitBufferFull)
			{
				Creator_Log(CreatorLogLevel_Info, "TCP socket read error %d \r\n", error);
				connectionLost = true;
			}
		}
		else if (receivedDataLength == 0)
		{
			Creator_Log(CreatorLogLevel_Debug, "TCP socket port %d read no data (socket closed) \r\n", controlBlock->ConnectionDestinationPort);
			connectionLost = true;
		}
	}
	else
	{
		errno = 0;
		receivedDataLength = recv(controlBlock->ConnectionHandle, receivedBuffer + controlBlock->PacketOffsetLength, maximumLengthToRead, 0);
		int lastError = errno;
		if (receivedDataLength == 0)
		{
			connectionLost = true;
		}
		else if (receivedDataLength == SOCKET_ERROR)
		{
			if (lastError == EWOULDBLOCK)
				receivedDataLength = 0;
			else if (lastError == ENOTCONN)
				connectionLost = true;
			else if (lastError == EBADF)
				connectionLost = true;
		}

	}
	CreatorCommonMessaging_UnLockTCP();

	if (receivedDataLength > 0)
	{
		// BEWARE: Must unlock TCP mutex to avoid deadlock caused by SIP response messages sent within callback!
		// TODO: protect against concurrent connection recovery if an asynchronous send fails...
		receivedDataLength += controlBlock->PacketOffsetLength;
		if (controlBlock->LengthOfRemainingContent > 0 && controlBlock->IsContentReadingInProgress)
		{
			COMMON_MESSAGING_LOG(CreatorLogLevel_Debug, "> RECV(%d) CONT len=%d", controlBlock->ConnectionHandle,
					receivedDataLength - controlBlock->PacketOffsetLength);
			maximumLengthToRead = CreatorCommonMessaging_HandleContent(controlBlock, receivedBuffer, &receivedDataLength);
			if (receivedDataLength > 0)
			{
				CreatorCommonMessaging_ParseMessage(controlBlock, receivedBuffer + maximumLengthToRead, receivedDataLength);
			}
		}
		else
		{
			COMMON_MESSAGING_LOG(CreatorLogLevel_Debug, "> RECV(%d) len=%d", controlBlock->ConnectionHandle, receivedDataLength);
			CreatorCommonMessaging_ParseMessage(controlBlock, receivedBuffer, receivedDataLength);
		}
		if(controlBlock->PacketOffsetLength == 0)
		{
			if(controlBlock->TemporaryDataBuffer != NULL)
					Creator_MemFree((void **)&controlBlock->TemporaryDataBuffer);
			controlBlock->TemporaryDataBufferLength = 0;
		}
	}

	if (!connectionLost && controlBlock->Enabled && controlBlock->ResponsePending && controlBlock->ResponseTimeout > 0)
	{
		// Check for response message timeout (note: check this even after data received in case it's garbage)
		if ((CreatorTimer_GetTickCount() - controlBlock->SendStartTime) >= controlBlock->ResponseTimeout)
		{
			Creator_Log(CreatorLogLevel_Error, "TCP failed - response timeout dest port=%d",
						controlBlock->ConnectionDestinationPort);
			connectionLost = true;
		}
	}
	
	if (connectionLost)
	{
		COMMON_MESSAGING_LOG(CreatorLogLevel_Info, "TCP(%d) - connection lost dest port=%d", controlBlock->ConnectionHandle, controlBlock->ConnectionDestinationPort);
		// Disable receive handling for the connection (use mutex to prevent recovery collision)
		if (controlBlock->Enabled)
		{
			controlBlock->Enabled = false;
			controlBlock->ResponsePending = false;

			// WARNINGS:
			// - Don't delete connection here otherwise the controlBlock could be released during an async send by other threads.
			// - The callback should abort any wait for response, and can disconnect or re-establish the connection
			//   BUT the client must protect against any send/recovery collisions from other threads
			if (controlBlock->ProtocolCallBack)
				controlBlock->ProtocolCallBack(CreatorCommonMessaging_CallbackEventType_NetworkFailure, NULL, NULL, 0, controlBlock->CallbackContext);
		}
	}
}


static void CreatorCommonMessaging_Task(CreatorThread thread, void * taskParameters)
{
	int controlBlockCount;
	CreatorCommonMessaging_ControlBlock *controlBlock;

	//if (!_IsCommonMessagingTaskCreated)
	//	return;

	for (;;)
	{
		if (_Shutdown)
			break;
        CreatorThread_SleepMilliseconds(NULL, 10);
		for (controlBlockCount = 0; controlBlockCount < CREATOR_MAX_CONNECTIONS; controlBlockCount++)
		{
			if (_Shutdown)
				break;
			controlBlock = &_CommonMessagingControlBlock[controlBlockCount];
			CreatorCommonMessaging_Receive(controlBlock);
		}
	}
}

void CreatorCommonMessaging_UnLockTCP(void)
{
#ifdef CMP_THREAD_SAFE
	if (!_TCPStackLock)
		_TCPStackLock = CreatorSemaphore_New(1, 0);
	CreatorSemaphore_Release(_TCPStackLock,1);
#endif
}
