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

#if CREATOR_CONFIG_CYASSL

/*! \file creator_tls.c
 *  \brief LibCreatorCore .
 */


#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
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

#define XMALLOC_USER

#include "creator_tls.h"
#include "cyassl/ssl.h"
#include "cyassl/version.h"
#ifdef LIBWOLFSSL_VERSION_HEX
#include "cyassl/error-ssl.h"
#else
#include "cyassl/error.h"
#endif

#include "cyassl/ctaocrypt/memory.h"

#include "creator/core/common_messaging_main.h"
#include "common_messaging_parser.h"
#include "creator/core/creator_threading.h"
#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_timer.h"
#include "creator/core/creator_debug.h"

typedef struct
{
	CYASSL *Session;
	CYASSL_CTX *Context;
} CyaSSL_Session;


static void FreeSession(CyaSSL_Session **session);
static int SSLReceiveCallBack(CYASSL *sslSessioon, char *recieveBuffer, int receiveBufferLegth, void *vp);
static int SSLSendCallBack(CYASSL *sslSessioon, char *sendBuffer, int sendBufferLength, void *vp);

#define WAIT_TIMEOUT_SECS					60	// wait timeout in seconds

#if 1
void *XMALLOC(size_t n, void* heap, int type)
{
	(void)heap;
	(void)type;
	return Creator_MemAlloc(n);
}

void *XREALLOC(void *p, size_t n, void* heap, int type)
{
	(void)heap;
	(void)type;
	return Creator_MemRealloc(p,n);
}

void XFREE(void *p, void* heap, int type)
{
	(void)heap;
	(void)type;
	Creator_MemFree(&p);
}
#endif

void CreatorTLS_Initialise(void)
{
	CyaSSL_Init();
	CyaSSL_SetAllocators(Creator_MemAlloc, Creator_MemSafeFree, Creator_MemRealloc);
}

void CreatorTLS_Shutdown(void)
{
	CyaSSL_Cleanup();
}

bool CreatorTLS_StartSSLSession(CreatorCommonMessaging_ControlBlock *controlBlock, bool client)
{
	bool result = false;
	CyaSSL_Session *session = Creator_MemAlloc(sizeof(CyaSSL_Session));
	if (session)
	{
		memset(session,0, sizeof(CyaSSL_Session));
		if (client)
			session->Context = CyaSSL_CTX_new(CyaTLSv1_client_method());
		else
			session->Context = CyaSSL_CTX_new(CyaTLSv1_server_method());
		if (session->Context)
		{
			if (client)
			{
				if (controlBlock->TLSCertificateData)
				{
					CyaSSL_CTX_load_verify_buffer(session->Context, controlBlock->TLSCertificateData, controlBlock->TLSCertificateSize, SSL_FILETYPE_PEM);
					CyaSSL_CTX_set_verify(session->Context, SSL_VERIFY_PEER, NULL);
				}
				else
					CyaSSL_CTX_set_verify(session->Context, SSL_VERIFY_NONE, NULL);
			}
			else
			{
				CyaSSL_CTX_use_certificate_buffer(session->Context, controlBlock->TLSCertificateData, controlBlock->TLSCertificateSize, SSL_FILETYPE_PEM);
				CyaSSL_CTX_use_PrivateKey_buffer(session->Context, controlBlock->TLSCertificateData, controlBlock->TLSCertificateSize, SSL_FILETYPE_PEM);
				CyaSSL_CTX_set_verify(session->Context, SSL_VERIFY_NONE, NULL);
			}
			CyaSSL_SetIORecv(session->Context, SSLReceiveCallBack);
			CyaSSL_SetIOSend(session->Context, SSLSendCallBack);
			session->Session = CyaSSL_new(session->Context);
			if (session->Session)
			{
				CyaSSL_set_fd(session->Session, controlBlock->ConnectionHandle);
				int timeOutPeriod = CreatorTimer_GetTicksPerSecond() * WAIT_TIMEOUT_SECS;
				uint startTick = CreatorTimer_GetTickCount();
				bool connectTimeout = false;
				while (1)
				{
					// TODO - want SSL/TCP thread safety, but not sure if safe to use tcp mutex here?
					CreatorCommonMessaging_LockTCP();
					if (client)
					{
						if (CyaSSL_connect(session->Session) == SSL_SUCCESS)
						{
							CreatorCommonMessaging_UnLockTCP();
							result = true;
							break;
						}
					}
					else
					{
						if (CyaSSL_accept(session->Session) == SSL_SUCCESS)
						{
							CreatorCommonMessaging_UnLockTCP();
							result = true;
							break;
						}												
					}
					CreatorCommonMessaging_UnLockTCP();
					int error;
					error = CyaSSL_get_error(session->Session, 0);
					if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE)
					{
						if (CreatorTimer_GetTickCount() - startTick >= timeOutPeriod)
						{
							connectTimeout=true;
							break;
						}
						CreatorThread_SleepMilliseconds(NULL, 5);
					}
					else
					{
						if ((error != SOCKET_ERROR_E) && (error != UNKNOWN_RECORD_TYPE) && (error != FATAL_ERROR) && (error != LENGTH_ERROR))
							Creator_Log(CreatorLogLevel_Error, "SSL connect failed - error = %d", error);
						break;
					}
				}
				if (connectTimeout)
				{
					Creator_Log(CreatorLogLevel_Error, "SSL connect failed - timeout");
				}

			}
			else
			{
				Creator_Log(CreatorLogLevel_Error, "SSL connect failed - CyaSSL_new");
			}
		}
		else
		{
			Creator_Log(CreatorLogLevel_Error, "SSL connect failed - CYASSL_CTX");
		}
		if (result)
		{
			CreatorCommonMessaging_LockTCP();
			controlBlock->SSLSession = session;
			CreatorCommonMessaging_UnLockTCP();
		}
		else
		{
			FreeSession(&session);
		}
	}
	return result;
}


void CreatorTLS_EndSSLSession(CreatorCommonMessaging_ControlBlock *controlBlock)
{
	FreeSession((CyaSSL_Session **)&controlBlock->SSLSession);
}

int CreatorTLS_Read(CreatorCommonMessaging_ControlBlock *controlBlock, void *buffer, size_t bufferSize)
{
	int result = 0;
	CyaSSL_Session *session = (CyaSSL_Session *)controlBlock->SSLSession;
	if (session)
	{
		result = CyaSSL_read(session->Session, buffer, bufferSize);
	}
	return result;

}

int CreatorTLS_Write(CreatorCommonMessaging_ControlBlock *controlBlock, void *buffer, size_t length)
{
	int result = 0;
	CyaSSL_Session *session = (CyaSSL_Session *)controlBlock->SSLSession;
	if (session)
	{
		result = CyaSSL_write(session->Session, buffer, length);
	}
	return result;
}

CreatorTLSError CreatorTLS_GetError(CreatorCommonMessaging_ControlBlock *controlBlock)
{
	CreatorTLSError result = CreatorTLSError_None;
	CyaSSL_Session *session = (CyaSSL_Session *)controlBlock->SSLSession;
	if (session)
	{
		int error = CyaSSL_get_error(session->Session, 0);
		switch (error)
		{
			case SSL_ERROR_WANT_READ:
				result = CreatorTLSError_RecieveBufferEmpty;
				break;
			case SSL_ERROR_WANT_WRITE:
				result = CreatorTLSError_TransmitBufferFull;
				break;
			case CYASSL_CBIO_ERR_CONN_CLOSE:
				result = CreatorTLSError_ConnectionClosed;
				break;
			case CYASSL_CBIO_ERR_CONN_RST:
				result = CreatorTLSError_ConnectionReset;
				break;

		}
	}
	return result;
}

static void FreeSession(CyaSSL_Session **session)
{
	CyaSSL_Session *self = *session;
	if (self)
	{
		if (self->Session != NULL)
		{
			CyaSSL_shutdown(self->Session);
			CyaSSL_free(self->Session);
			self->Session = NULL;
		}
		if (self->Context != NULL)
		{
			CyaSSL_CTX_free(self->Context);
			self->Context = NULL;
		}
		Creator_MemFree((void **)session);
	}
}


/* custom receive callback for CyaSSL, registered with CyaSSL_SetIORecv() */
static int SSLReceiveCallBack(CYASSL *sslSessioon, char *recieveBuffer, int receiveBufferLegth, void *vp)
{
	int result;
	SOCKET socket = CyaSSL_get_fd(sslSessioon);
	errno = 0;
	result =  recv(socket, recieveBuffer, receiveBufferLegth, 0);
	int lastError = errno;
	if (result == 0)
	{
		result = CYASSL_CBIO_ERR_CONN_CLOSE;
	}
	else if (result == SOCKET_ERROR)
	{
		if (lastError == EWOULDBLOCK)
			result = CYASSL_CBIO_ERR_WANT_READ;  // WANT_READ
		else if (lastError == ENOTCONN)
			result = CYASSL_CBIO_ERR_CONN_CLOSE;
		else if (lastError == EBADF)
			result = CYASSL_CBIO_ERR_CONN_CLOSE;
	}
	return result;
}

/* custom send callback for CyaSSL, registered with CyaSSL_SetIOSend() */
static int SSLSendCallBack(CYASSL *sslSessioon, char *sendBuffer, int sendBufferLength, void *vp)
{
	int result;
	SOCKET socket = CyaSSL_get_fd(sslSessioon);
	errno = 0;
	result = send(socket, sendBuffer, sendBufferLength, 0);
	int lastError = errno;
	if (result == SOCKET_ERROR)
	{
		if (lastError == EWOULDBLOCK)
			result = CYASSL_CBIO_ERR_WANT_WRITE;  // WANT_READ
		else if (lastError == ENOTCONN)
			result = CYASSL_CBIO_ERR_CONN_CLOSE;
		else if (lastError == ECONNRESET)
			result = CYASSL_CBIO_ERR_CONN_RST;
		else if (lastError == EBADF)
			result = CYASSL_CBIO_ERR_CONN_CLOSE;
	}
	return result;
}

#endif
