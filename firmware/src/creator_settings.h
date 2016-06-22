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
  Creator related settings

  File Name:
    creator_settings.h

  Summary:
    Creator-related configruation defines and settings.

  Description:

 *******************************************************************************/


#ifndef __CREATOR_SETTINGS_H_
#define	__CREATOR_SETTINGS_H_


#include <stdbool.h>
#include <stdint.h>

#define CREATOR_OAUTH_KEY_LENGTH		(16)
#define CREATOR_OAUTH_SECRET_LENGTH	(16)

#define CREATOR_BLANK_DEVICE_NAME		"NoDeviceNameSet"
#ifdef MICROCHIP_PIC32
#define CREATOR_DEFAULT_DEVICE_TYPE	"WiFire"
#else
#define CREATOR_DEFAULT_DEVICE_TYPE	"ci20"
#endif
#define CREATOR_DEFAULT_REST_ROOT_URL	"https://ws-uat.creatorworld.com"
#define CREATOR_DEFAULT_OAUTH_KEY		"Ph3bY5kkU4P6vmtT"
#define CREATOR_DEFAULT_OAUTH_SECRET	"Sd1SVBfYtGfQvUCR"



#endif	/* __CREATOR_SETTINGS_H_ */

