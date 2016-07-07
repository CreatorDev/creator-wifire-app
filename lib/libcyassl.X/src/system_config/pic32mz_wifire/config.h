/***********************************************************************************************************************
 Copyright (c) 2016, Imagination Technologies Limited
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

#ifndef CONFIG_H
#define	CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

#if defined(INLINE)
#undef INLINE
#define INLINE inline
#endif
	
//#define DEBUG_WOLFSSL	
#define XMALLOC_USER
#define WOLFSSL_HAVE_MIN
#define WOLFSSL_HAVE_MAX

#define MICROCHIP_PIC32_RNG
#define NEED_AES_TABLES
#define SIZEOF_LONG_LONG 8
#define WOLFSSL_USER_IO
#define NO_WRITEV
#define NO_DEV_RANDOM
#define NO_FILESYSTEM
#define WOLFSSL_STATIC_RSA
#define NO_RC4
//#define NO_RSA

#define USE_FAST_MATH
#define TFM_TIMING_RESISTANT
	
#define WOLFSSL_DTLS 1   
#define HAVE_AESCCM
#define HAVE_ECC 1
#define HAVE_ECC_ENCRYPT 1
#define HAVE_HKDF 1
#define LARGE_STATIC_BUFFERS 1

#define BUILD_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8
#define BUILD_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256
#define BUILD_TLS_PSK_WITH_AES_128_CCM_8
#define BUILD_TLS_PSK_WITH_AES_128_CBC_SHA256

#define BUILD_TLS_RSA_WITH_AES_256_CBC_SHA


// Use FlowTime instead of SNTP client
// Note: SNTP client caused problems on WiFire with Harmony v0_80b. The SNTP client caused DNS requests to fail
// when WiFi host was on stand-alone laptop and this that made CommonMessaging requests fail.
//#include "flow_time.h"
#define TCPIP_SNTP_UTCSecondsGet(t1)	Creator_GetTime(t1)
	
#ifdef	__cplusplus
}
#endif

#endif	/* CONFIG_H */
