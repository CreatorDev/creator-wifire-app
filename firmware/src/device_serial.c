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

/*! \file device_serial.c
 *  \brief Module for retrieving the device's serial number
 */

#include <stdint.h>
#include <stdbool.h>
#ifdef MICROCHIP_PIC32
	#include <xc.h>
	#include "system/debug/sys_debug.h"
	#include "system/console/sys_console.h"
#endif

#include "device_serial.h"


uint64_t DeviceSerial_GetCpuSerialNumberUint64(void)
{
#ifdef MICROCHIP_PIC32
	return ((uint64_t) DEVSN1 << 32 | (uint64_t) DEVSN0);
#else
	return 0;
#endif
}

bool DeviceSerial_GetCpuSerialNumberHexString (char* buffer, uint32_t buffSize)
{
	#define HEX_SN_SIZE_CHARS	(16)

	bool result = false;
#ifdef MICROCHIP_PIC32
	if (buffer && buffSize >= HEX_SN_SIZE_CHARS+1)
	{
		uint8_t  i = 0;
		for (i = 0; i < HEX_SN_SIZE_CHARS/2; i++)
			sprintf((char*)buffer+(i*2*sizeof(char)), "%02X", 0xFF);

		uint64_t serialNumber = ((uint64_t) DEVSN1 << 32 | (uint64_t) DEVSN0);
		uint8_t* _serialNumber = (uint8_t*) &serialNumber;
		memset(buffer, 0, sizeof(char) * (HEX_SN_SIZE_CHARS+1) );
		for (i = 0; i < HEX_SN_SIZE_CHARS/2; i++)
			sprintf((char*)buffer+(i*2*sizeof(char)), "%02X", *(_serialNumber+i));

		result = true;
	}
#else
#define APP_DEFAULT_DEVICE_SERIAL_NUMBER "123456"
	sprintf((char*)buffer, APP_DEFAULT_DEVICE_SERIAL_NUMBER);
	result = true;
#endif
	return result;
}
