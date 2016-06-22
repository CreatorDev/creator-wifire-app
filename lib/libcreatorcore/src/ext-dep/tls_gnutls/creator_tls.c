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

#if CREATOR_CONFIG_GNUTLS

/*! \file creator_tls.c
 *  \brief LibCreatorCore .
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#ifdef MICROCHIP_PIC32
	#include <tcpip/berkeley_api.h>
	#include "tcpip/src/system/errno.h"
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

#include "creator_tls.h"
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "creator/core/common_messaging_main.h"
#include "common_messaging_parser.h"
#include "creator/core/creator_threading.h"
#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_timer.h"
#include "creator/core/creator_debug.h"

typedef struct
{
	int32 ConnectionHandle;
	gnutls_session_t Session;
	gnutls_certificate_credentials_t Credentials;
	CreatorTLSError Error;
} GnuTLS_Session;

#define WAIT_TIMEOUT_SECS					60	// wait timeout in seconds

static void FreeSession(GnuTLS_Session **session);
static int ReceiveTimeout(gnutls_transport_ptr_t context, unsigned int ms);
static ssize_t SSLReceiveCallBack(gnutls_transport_ptr_t context, void *recieveBuffer, size_t receiveBufferLegth);
static ssize_t SSLSendCallBack(gnutls_transport_ptr_t context, const void * sendBuffer,size_t sendBufferLength);
static int CertificateVerify(gnutls_session_t session);

//Comment out as init of DH params takes a while
//static gnutls_dh_params_t _DHParameters;
static gnutls_priority_t _PriorityCache;


void CreatorTLS_Initialise(void)
{
	gnutls_global_init();
//    unsigned int bits = gnutls_sec_param_to_pk_bits(GNUTLS_PK_DH, GNUTLS_SEC_PARAM_LEGACY);
//    gnutls_dh_params_init(&_DHParameters);
//    gnutls_dh_params_generate2(_DHParameters, bits);
    gnutls_priority_init(&_PriorityCache, "PERFORMANCE:%SERVER_PRECEDENCE", NULL);
}

void CreatorTLS_Shutdown(void)
{
//	gnutls_dh_params_deinit(_DHParameters);
	gnutls_priority_deinit(_PriorityCache);
	gnutls_global_deinit();
}

bool CreatorTLS_StartSSLSession(CreatorCommonMessaging_ControlBlock *controlBlock, bool client)
{
	bool result = false;
	GnuTLS_Session *session = Creator_MemAlloc(sizeof(GnuTLS_Session));
	if (session)
	{
		memset(session,0,sizeof(GnuTLS_Session));
		unsigned int flags;
#if GNUTLS_VERSION_MAJOR >= 3
		if (client)
			flags = GNUTLS_CLIENT | GNUTLS_NONBLOCK;
		else
			flags = GNUTLS_SERVER | GNUTLS_NONBLOCK;
		if (gnutls_init(&session->Session, flags) == GNUTLS_E_SUCCESS)
#else
		if (client)
			flags = GNUTLS_CLIENT;
		else
			flags = GNUTLS_SERVER;
		if (gnutls_init(&session->Session, flags) == GNUTLS_E_SUCCESS)
#endif
		{
			session->ConnectionHandle = controlBlock->ConnectionHandle;
			gnutls_transport_set_pull_function(session->Session, SSLReceiveCallBack);
			gnutls_transport_set_push_function(session->Session, SSLSendCallBack);
#if GNUTLS_VERSION_MAJOR >= 3
            gnutls_transport_set_pull_timeout_function(session->Session, ReceiveTimeout);
#endif
			gnutls_transport_set_ptr(session->Session, session);
			if (gnutls_certificate_allocate_credentials(&session->Credentials) == GNUTLS_E_SUCCESS)
			{
				if (controlBlock->TLSCertificateData)
				{
					gnutls_datum_t certificateData;
					certificateData.data = controlBlock->TLSCertificateData;
					certificateData.size = controlBlock->TLSCertificateSize;
					if (client)
						gnutls_certificate_set_x509_trust_mem(session->Credentials, &certificateData, GNUTLS_X509_FMT_PEM);
					else
						gnutls_certificate_set_x509_key_mem(session->Credentials, &certificateData, &certificateData, GNUTLS_X509_FMT_PEM);
				}
				else
				{
#if GNUTLS_VERSION_MAJOR >= 3
					gnutls_certificate_set_verify_function(session->Credentials,CertificateVerify);
					//gnutls_certificate_set_retrieve_function(xcred, cert_callback);
					//gnutls_session_set_verify_cert(session->Session, NULL, GNUTLS_VERIFY_DISABLE_CA_SIGN);
#else
					gnutls_certificate_set_verify_flags(session->Credentials, GNUTLS_VERIFY_DISABLE_CA_SIGN);
#endif
				}
				gnutls_credentials_set(session->Session, GNUTLS_CRD_CERTIFICATE, session->Credentials);
			}
			if (client)
				gnutls_set_default_priority(session->Session);
			else
			{
//		        gnutls_certificate_set_dh_params(session->Credentials, _DHParameters);
		        gnutls_priority_set(session->Session, _PriorityCache);
                gnutls_certificate_server_set_request(session->Session, GNUTLS_CERT_IGNORE); //Don't require Client Cert
			}

			int timeOutPeriod = CreatorTimer_GetTicksPerSecond() * WAIT_TIMEOUT_SECS;
			uint startTick = CreatorTimer_GetTickCount();
			bool connectTimeout = false;
#if GNUTLS_VERSION_MAJOR >= 3
			gnutls_handshake_set_timeout(session->Session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);
#endif
			while (1)
			{
				// TODO - want SSL/TCP thread safety, but not sure if safe to use tcp mutex here?
				CreatorCommonMessaging_LockTCP();
				int handshakeResult = gnutls_handshake(session->Session);
				if (handshakeResult == GNUTLS_E_SUCCESS)
				{
					CreatorCommonMessaging_UnLockTCP();
					result = true;
					break;
				}
				CreatorCommonMessaging_UnLockTCP();
				if (gnutls_error_is_fatal(handshakeResult) != 0)
				{
					break;
				}
				int error;
				error = session->Error;
				if (error == CreatorTLSError_RecieveBufferEmpty || error == CreatorTLSError_TransmitBufferFull)
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
					Creator_Log(CreatorLogLevel_Error, "SSL connect failed - error = %d", error);
					break;
				}
			}
			if (connectTimeout)
			{
				Creator_Log(CreatorLogLevel_Error, "SSL connect failed - timeout");
			}
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
	FreeSession((GnuTLS_Session **)&controlBlock->SSLSession);
}

int CreatorTLS_Read(CreatorCommonMessaging_ControlBlock *controlBlock, void *buffer, size_t bufferSize)
{
	int result = 0;
	if (controlBlock->SSLSession)
	{
		GnuTLS_Session *session = (GnuTLS_Session*)controlBlock->SSLSession;
		result = gnutls_read(session->Session, buffer, bufferSize);
	}
	return result;
}

int CreatorTLS_Write(CreatorCommonMessaging_ControlBlock *controlBlock, void *buffer, size_t length)
{
	int result = 0;
	if (controlBlock->SSLSession)
	{
		GnuTLS_Session *session = (GnuTLS_Session*)controlBlock->SSLSession;
		result = gnutls_write(session->Session, buffer, length);
	}
	return result;
}

CreatorTLSError CreatorTLS_GetError(CreatorCommonMessaging_ControlBlock *controlBlock)
{
	CreatorTLSError result = CreatorTLSError_None;
	GnuTLS_Session *session = (GnuTLS_Session*)controlBlock->SSLSession;
	if (session)
	{
		result = session->Error;
	}
	return result;
}

static int CertificateVerify(gnutls_session_t session)
{
	return 0;
}

static void FreeSession(GnuTLS_Session **session)
{
	GnuTLS_Session *self = *session;
	if (self)
	{
		if (self->Session)
			gnutls_deinit(self->Session);
		if (self->Credentials)
			gnutls_certificate_free_credentials(self->Credentials);
		Creator_MemFree((void **)session);
	}
}


static int ReceiveTimeout(gnutls_transport_ptr_t context, unsigned int ms)
{
//	fd_set rfds;
//	struct timeval tv;
//	int ret;
//	GnuTLS_Session *session = (GnuTLS_Session*)context;
//	int fd = session->ConnectionHandle;
//
//	FD_ZERO(&rfds);
//	FD_SET(fd, &rfds);
//
//	tv.tv_sec = ms/1000;
//	tv.tv_usec = (ms % 1000) * 1000;
//
//	ret = select(fd + 1, &rfds, NULL, NULL, &tv);
//	if (ret <= 0)
//		return ret;
//	return ret;
	return 1;
}

static ssize_t SSLReceiveCallBack(gnutls_transport_ptr_t context, void *recieveBuffer, size_t receiveBufferLegth)
{
	ssize_t result;
	GnuTLS_Session *session = (GnuTLS_Session*)context;
	SOCKET socket = session->ConnectionHandle;
	errno = 0;
	session->Error = CreatorTLSError_None;
	result =  recv(socket, recieveBuffer, receiveBufferLegth, 0);
	int lastError = errno;
	if (result == 0)
	{
		session->Error = CreatorTLSError_ConnectionClosed;
	}
	else if (result == SOCKET_ERROR)
	{
		if (lastError == EWOULDBLOCK)
		{
			session->Error = CreatorTLSError_RecieveBufferEmpty;
			gnutls_transport_set_errno(session->Session, EAGAIN);
		}
		else if (lastError == ENOTCONN)
			session->Error = CreatorTLSError_ConnectionClosed;
		else if (lastError == EBADF)
			session->Error = CreatorTLSError_ConnectionClosed;
	}
	return result;
}

static ssize_t SSLSendCallBack(gnutls_transport_ptr_t context, const void * sendBuffer,size_t sendBufferLength)
{
	ssize_t result;
	GnuTLS_Session *session = (GnuTLS_Session*)context;
	SOCKET socket = session->ConnectionHandle;
	errno = 0;
	session->Error = CreatorTLSError_None;
	result = send(socket, sendBuffer, sendBufferLength, 0);
	int lastError = errno;
	if (result == SOCKET_ERROR)
	{
		if (lastError == EWOULDBLOCK)
			session->Error = CreatorTLSError_TransmitBufferFull;
		else if (lastError == ENOTCONN)
			session->Error = CreatorTLSError_ConnectionClosed;
		else if (lastError == ECONNRESET)
			session->Error = CreatorTLSError_ConnectionReset;
		else if (lastError == EBADF)
			session->Error = CreatorTLSError_ConnectionClosed;
	}
	return result;
}


#endif
