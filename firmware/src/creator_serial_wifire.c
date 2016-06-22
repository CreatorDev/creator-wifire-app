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

/*
 * CreatorSerial implementation for WiFire
 *
 */


#include "creator/creator_serial.h"

#include <stdarg.h>
#include <p32xxxx.h>
#include <xc.h>
#include <cp0defs.h>
#include "system_config.h"
#include "peripheral/usart/plib_usart.h"
#include "system/clk/sys_clk.h"
#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_threading.h"

#define CREATOR_SERIAL_BUFFER_LEN		(1024)

// this serial implementation could be improved to use a circular
// buffer for input, and read into this buffer using an interrupt.
// Currently we use Harmony's USART implementation which as of
// 02/2015 only polls the registers when data is attempted to be
// read from the device



CreatorSerialDevice CreatorSerial_Init(int port, unsigned int baud)
{
	CreatorSerialDevice result;
	USART_MODULE_ID usartID = (port-1);
	uint32_t clockSource = SYS_CLK_PeripheralFrequencyGet ( CLK_BUS_PERIPHERAL_1 );

	PLIB_USART_InitializeOperation(usartID,USART_RECEIVE_FIFO_ONE_CHAR,USART_TRANSMIT_FIFO_IDLE,USART_ENABLE_TX_RX_USED);
	
	int32_t baudLow  = ( (clockSource/baud) >> 4 ) - 1;
	int32_t baudHigh = ( (clockSource/baud) >> 2 ) - 1;
#if defined (PLIB_USART_ExistsModuleBusyStatus)
	isEnabled = PLIB_USART_ModuleIsBusy (usartID);
	if (isEnabled)
	{
		PLIB_USART_Disable (usartID);
		while (PLIB_USART_ModuleIsBusy (usartID));
	}
#endif		

	if ((baudHigh >= 0) && (baudHigh <= UINT16_MAX))
	{
		PLIB_USART_BaudRateHighEnable(usartID);
		PLIB_USART_BaudRateHighSet(usartID,clockSource,baud);
	}
	else if ((baudLow >= 0) && (baudLow <= UINT16_MAX))
	{
		PLIB_USART_BaudRateHighDisable(usartID);
		PLIB_USART_BaudRateSet(usartID, clockSource, baud);   
	}		
	PLIB_USART_LineControlModeSelect(usartID, USART_8N1);		
	PLIB_USART_Enable(usartID);
	PLIB_USART_ReceiverEnable(usartID);
	PLIB_USART_TransmitterEnable(usartID);
	result = (CreatorSerialDevice)usartID;
	return result;
}

void CreatorSerial_Write(CreatorSerialDevice device, const char* text, int length)
{
	USART_MODULE_ID usartID = (USART_MODULE_ID)device;
	int total = 0;
    do
    {
		if (!PLIB_USART_TransmitterBufferIsFull(usartID))
		{
			PLIB_USART_TransmitterByteSend(usartID, text[total]);
			total +=1;
		}
//		if (total < length)
//			CreatorThread_SleepMilliseconds(NULL,0);
    } while( total < length );
}

void CreatorSerial_Printf(CreatorSerialDevice device, const char* format, ...)
{
	va_list arg_list;
	char buff[CREATOR_SERIAL_BUFFER_LEN];
	va_start(arg_list, format);
	int length = vsnprintf(buff, CREATOR_SERIAL_BUFFER_LEN, format, arg_list);
	va_end(arg_list);
	CreatorSerial_Write(device, buff, length);
}

void CreatorSerial_Puts(CreatorSerialDevice device, const char* msg)
{
	int length = strlen(msg);
	CreatorSerial_Write(device, msg, length);
}

bool CreatorSerial_Ready(CreatorSerialDevice device)
{
	bool result = false;
	USART_MODULE_ID usartID = (USART_MODULE_ID)device;
	if (PLIB_USART_ReceiverOverrunHasOccurred(usartID))
	{
		PLIB_USART_ReceiverOverrunErrorClear(usartID);		
	}
	result = PLIB_USART_ReceiverDataIsAvailable(usartID);
	return result;
}

char CreatorSerial_Getc(CreatorSerialDevice device)
{
	char result = '\0';
	USART_MODULE_ID usartID = (USART_MODULE_ID)device;
	while (!PLIB_USART_ReceiverDataIsAvailable(usartID));
	result = (char)PLIB_USART_ReceiverByteReceive(usartID);
	return result;
}

void CreatorSerial_Putc(CreatorSerialDevice device, char c)
{
	USART_MODULE_ID usartID = (USART_MODULE_ID)device;
	while (true)
	{
		if (!PLIB_USART_TransmitterBufferIsFull(usartID))
		{
			PLIB_USART_TransmitterByteSend(usartID, c);
			break;
		}
		//CreatorThread_SleepMilliseconds(NULL,0);
	}
}

int CreatorSerial_Getline(CreatorSerialDevice device, char* line, int bufferLen)
{
	int result = 0;
    do
    {
        *line = CreatorSerial_Getc(device);
    }while((*line++ != '\n') && (result++ < bufferLen));

	return result;
}
