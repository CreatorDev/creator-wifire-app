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

/*! \file arduino_monitor.c
 *  \brief Task used for monitoring comms from the Arduino Shield serial channel
 */

#include <xc.h>

#include "arduino_monitor.h"

#include "app.h"
#include "command_handlers.h"

// Creator library APIs
#include "creator/creator_console.h"
#include "creator/creator_serial.h"
#include "creator/core/creator_threading.h"
#include "string_builder.h"

// TODO - consider hooking up to generic lwm2m command request/response object...

static CreatorSerialDevice arduinoSerial;

static unsigned int cmdRequestID = 0;

/*********************************************************************
 * Function:        void SYS_ArduinoMonitorTask(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Receive and monitor communications with the Arduino-shield serial comms channel
 *
 * Side Effects:    None
 *
 * Overview:        This function implements the task for monitoring comms from the Arduino shield.
 *
 * Note:            None
 ********************************************************************/
void SYS_ArduinoMonitorTask(CreatorThread thread, void *taskParameters)
{
    char incomingChar = 0;

#define ARDUINO_SERIAL_PORT 6
#define ARDUINO_SERIAL_BAUD 115200

    // power the PMODRS232
    /*
     * UART 6 - Shield RS232 port
     * - u6Txd on pin RPB15
     * - u6RXd on pin RPC2
     */

    // TXd Pin setup.
    PLIB_PORTS_PinModePerPortSelect(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_15, PORTS_PIN_MODE_DIGITAL);
    PLIB_PORTS_PinDirectionOutputSet(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_15);
    RPB15R = 0x04;

    // RXd Pin setup.
    PLIB_PORTS_PinModePerPortSelect(PORTS_ID_0, PORT_CHANNEL_C, PORTS_BIT_POS_2, PORTS_PIN_MODE_DIGITAL);
    PLIB_PORTS_PinDirectionInputSet(PORTS_ID_0, PORT_CHANNEL_C, PORTS_BIT_POS_2);
    U6RXR = 0x0C;

    // RPG8 3V3 supply Digilent PMODRS232
    PLIB_PORTS_PinModePerPortSelect(PORTS_ID_0, PORT_CHANNEL_G, PORTS_BIT_POS_8, PORTS_PIN_MODE_DIGITAL);
    PLIB_PORTS_PinDirectionOutputSet(PORTS_ID_0, PORT_CHANNEL_G, PORTS_BIT_POS_8);
    PLIB_PORTS_PinSet(PORTS_ID_0, PORT_CHANNEL_G, PORTS_BIT_POS_8);

    // RPG7 0V supply to Digilent PMODRS232
    PLIB_PORTS_PinModePerPortSelect(PORTS_ID_0, PORT_CHANNEL_G, PORTS_BIT_POS_7, PORTS_PIN_MODE_DIGITAL);
    PLIB_PORTS_PinDirectionOutputSet(PORTS_ID_0, PORT_CHANNEL_G, PORTS_BIT_POS_7);
    PLIB_PORTS_PinClear(PORTS_ID_0, PORT_CHANNEL_G, PORTS_BIT_POS_7);

    arduinoSerial = CreatorSerial_Init(ARDUINO_SERIAL_PORT, ARDUINO_SERIAL_BAUD);

#define ARDUINO_CMD_BUFFER_LENGTH 255
    char commandBuffer[ARDUINO_CMD_BUFFER_LENGTH + 1];
    char* cursor = commandBuffer;
    unsigned int freeBufferSize = ARDUINO_CMD_BUFFER_LENGTH;
    bool processCommand = false;

    memset(commandBuffer, '\0', ARDUINO_CMD_BUFFER_LENGTH + 1);

    while (1)
    {
        while (CreatorSerial_Ready(arduinoSerial))
        {
            incomingChar = CreatorSerial_Getc(arduinoSerial);

            if (freeBufferSize > 0 && incomingChar != '\n' && incomingChar != '\r')
            {
                *cursor = incomingChar;
                freeBufferSize--;
                cursor++;
            }
            else
            {
                *cursor = '\0';
                processCommand = true;
            }

            if (processCommand)
            {
                cmdRequestID++;
                // TODO - drop Arduino task? Discuss - previous role, c.f. LWM2M project...
//				if (PublishCommandToOwner(cmdRequestID, commandBuffer))
//				{
//					CreatorConsole_Printf("\n\rSuccessfully forwarded command received from Arduino-shield comms interface ('%s').\n\r", commandBuffer);
//				}
//				else
//				{
                CreatorConsole_Puts("\n\rWarning: Could not forward command received from Arduino-shield comms interface.");
//				}
                processCommand = false;
                cursor = commandBuffer;
                freeBufferSize = ARDUINO_CMD_BUFFER_LENGTH;
            }
        }
        CreatorThread_SleepMilliseconds(NULL, 100);
    }
}

void Arduino_SendCommand(const char* command)
{
    CreatorSerial_Puts(arduinoSerial, command);
}
