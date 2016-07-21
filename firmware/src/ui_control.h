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
  User Interface Control Headers

  File Name:
    ui_control.h

  Summary:
    APIs for manipulating the user interface behaviour of the Creator Starter App

  Description:

 *******************************************************************************/

#ifndef __UI_CONTROL_H_
#define __UI_CONTROL_H_

#include <stdbool.h>
#include <stdint.h>


#define NUMBER_OF_LEDS (4)

typedef enum
{
    UILEDState_Off = 0,
    UILEDState_On
} UILEDState;

typedef enum
{
    UILEDMode_Manual = 0,   // Manually controlled (UI controller module will not modify the LED's state)
    UILEDMode_Off,
    UILEDMode_On,
    UILEDMode_FlashStartOn,
    UILEDMode_FlashStartOff,
    UILEDMode_Max,
} UILEDMode;


typedef enum
{
    AppUIState_None = 0,
    AppUIState_SoftApHardwareError,
    AppUIState_SoftApNotConnected,
    AppUIState_SoftApConnected,
    AppUIState_SoftApNotConfigured,
    AppUIState_SoftApConfigured,
    AppUIState_AppInitConnectingToNetwork,
    AppUIState_AppInitConnectedToNetwork,
    AppUIState_AppInitConnectingToServer,
    AppUIState_AppConnectedToServer,
    AppUIState_AppHardwareError,
    AppUIState_Max

} AppUIState;

void UIControl_UIStep(void);

// APIs
void UIControl_ClearLEDs(void);
bool UIControl_SetUIState(AppUIState newState);
bool UIControl_SetLEDMode(uint8_t led, UILEDMode newMode);
bool UIControl_SetLEDState(uint8_t led, UILEDState newState);
bool UIControl_LEDCommand(int led, bool ledOn);         // user console command

void UIControl_pollInputSensors(void);

#endif // __UI_CONTROL_H_
