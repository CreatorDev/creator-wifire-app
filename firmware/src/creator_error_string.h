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


/*******************************************************************************
  creator error headers

  File Name:
    CreatorError.h

  Summary:
    Error description strings using error numbers.

  Description:

 *******************************************************************************/
#ifndef __CREATOR_ERROR_STRING_H_
#define __CREATOR_ERROR_STRING_H_


typedef enum 
{
    INTERNAL_ERROR = 1,
    INSUFFICIENT_MEMORY,	    //2
    METHOD_UNAVAILABLE,		    //3
    INVALID_ARGUMENT,		    //4
    RESOURCE_NOT_AVAILABLE,	    //5
    NETWORK_ERROR,			    //6
    UNAUHORIZED,			    //7
    CONFLICTS_DATA,			    //8
    RESOURCE_REMOVED,		    //9
    INTERNAL_SERVER_ERROR,	    //10
    SERVER_BUSY,			    //11
    SERVER_TIMEOUT,			    //12
    CLIENT_NOT_LOGGEDIN,	    //13
    CLIENT_NOT_COMPATIBLE = 15	//15
} ERROR_NUM;

#endif //__CREATOR_ERROR_H_

