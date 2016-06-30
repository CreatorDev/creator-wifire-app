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

/*! \file ui_control.c
 *  \brief APIs for user interface in Creator WiFire App
 */

#include "button_object.h"
#include "temperature_object.h"
#include "ui_control.h"

#ifdef MICROCHIP_PIC32
#include "bsp_config.h"
#endif

#ifndef LED_UPDATE_IMMEDIATE
#define LED_UPDATE_IMMEDIATE 1
#endif

typedef struct
{
#ifdef MICROCHIP_PIC32
    BSP_LED		LedId;
#else
    bool 		StateChanged;
#endif
    UILEDState	State;
    UILEDMode	Mode;

} LedControlConfig;


static LedControlConfig leds[MAX_LEDS] =
{
#ifdef MICROCHIP_PIC32
    {BSP_LED_1, UILEDState_Off, UILEDMode_Manual},		// BSP LED 1
    {BSP_LED_2, UILEDState_Off, UILEDMode_Manual},		// BSP LED 2
    {BSP_LED_3, UILEDState_Off, UILEDMode_Manual},		// BSP LED 3
#ifndef CREATOR_PIC32MZ_ETHERNET
    {BSP_LED_4, UILEDState_Off, UILEDMode_Manual},		// BSP LED 4
#endif
#else
    {false, UILEDState_Off, UILEDMode_Manual},		// BSP LED 1
    {false, UILEDState_Off, UILEDMode_Manual},		// BSP LED 2
    {false, UILEDState_Off, UILEDMode_Manual},		// BSP LED 3
    {false, UILEDState_Off, UILEDMode_Manual},		// BSP LED 4
#endif
};


void UIControl_ClearLEDs(void)
{
#ifdef MICROCHIP_PIC32

    UIControl_SetLEDMode(1, UILEDMode_Manual);
    UIControl_SetLEDMode(2, UILEDMode_Manual);
    UIControl_SetLEDMode(3, UILEDMode_Manual);
    UIControl_SetLEDMode(4, UILEDMode_Manual);

    UIControl_SetLEDState(1, UILEDState_Off);
    UIControl_SetLEDState(2, UILEDState_Off);
    UIControl_SetLEDState(3, UILEDState_Off);
    UIControl_SetLEDState(4, UILEDState_Off);
#else
    UIControl_SetLEDMode(4, UILEDMode_Manual);
    UIControl_SetLEDState(4, UILEDState_Off);
#endif
}

bool UIControl_SetLEDMode(uint8_t led, UILEDMode newMode)
{
    bool result = false;
    if (led > 0 && led <= MAX_LEDS)
    {
        leds[led - 1].Mode = newMode;
        switch (newMode) {
            case UILEDMode_Off:
            case UILEDMode_FlashStartOff:
#ifndef MICROCHIP_PIC32
                if (leds[led - 1].State != UILEDState_Off)
                    leds[led - 1].StateChanged = true;
#endif
                leds[led - 1].State = UILEDState_Off;
                break;

            case UILEDMode_On:
            case UILEDMode_FlashStartOn:
#ifndef MICROCHIP_PIC32
                if (leds[led - 1].State != UILEDState_On)
                    leds[led - 1].StateChanged = true;
#endif
                leds[led - 1].State = UILEDState_On;
                break;

            case UILEDMode_Manual:
            default:
                break;
        }
        result = true;
    }
    return result;
}

bool UIControl_SetLEDState(uint8_t led, UILEDState newState)
{
    bool result = false;
    if (led > 0 && led <= MAX_LEDS)
    {
#ifndef MICROCHIP_PIC32
        if (leds[led - 1].State != newState)
            leds[led - 1].StateChanged = true;
#endif
        leds[led - 1].State = newState;

#if LED_UPDATE_IMMEDIATE
        // Don't wait for task to call UIStep()
        if (leds[led - 1].Mode == UILEDMode_Manual)
        {
            if (leds[led - 1].State == UILEDState_On)
                BSP_LEDStateSet(leds[led - 1].LedId, BSP_LED_STATE_ON);
            else
                BSP_LEDStateSet(leds[led - 1].LedId, BSP_LED_STATE_OFF);
        }
#endif
        result = true;
    }
    return result;
}

void UIControl_UIStep(void)
{
    uint8_t led;
    for (led = 1; led <= MAX_LEDS; led++)
    {
        UILEDMode ledMode = leds[led - 1].Mode;

        switch (ledMode) {
            case UILEDMode_On:
#ifndef MICROCHIP_PIC32
                if (leds[led - 1].State != UILEDState_On)
                    leds[led - 1].StateChanged = true;
#endif
                leds[led - 1].State = UILEDState_On;
                break;

            case UILEDMode_Off:
#ifndef MICROCHIP_PIC32
                if (leds[led - 1].State != UILEDState_Off)
                    leds[led - 1].StateChanged = true;
#endif
                leds[led - 1].State = UILEDState_Off;
                break;

            case UILEDMode_FlashStartOn:
            case UILEDMode_FlashStartOff:
            {
#ifdef MICROCHIP_PIC32
                if (leds[led-1].State == UILEDState_Off)
                leds[led-1].State = UILEDState_On;
                else
                leds[led-1].State = UILEDState_Off;
#else
                if (leds[led - 1].State == UILEDState_Off)
                {
                    leds[led - 1].State = UILEDState_On;
                    leds[led - 1].StateChanged = true;
                }
                else
                {
                    leds[led - 1].State = UILEDState_Off;
                    leds[led - 1].StateChanged = true;
                }
#endif
            }
                break;

            case UILEDMode_Manual:
            default:
                break;
        }
    }

    for (led = 1; led <= MAX_LEDS; led++)
    {
#ifdef MICROCHIP_PIC32
        if(leds[led-1].State == UILEDState_On)
        BSP_LEDStateSet(leds[led-1].LedId, BSP_LED_STATE_ON);
        else
        BSP_LEDStateSet(leds[led-1].LedId, BSP_LED_STATE_OFF);
#else
        if (leds[led - 1].StateChanged == true)
        {
            if (leds[led - 1].State == UILEDState_On)
                SetLED(led, 1);
            else
                SetLED(led, 0);
            leds[led - 1].StateChanged = false;
        }
#endif
    }
}

bool UIControl_SetUIState(AppUIState newState)
{
    bool result = false;
    if (newState < AppUIState_Max)
    {
        switch (newState) {
            case AppUIState_SoftApHardwareError:
                UIControl_SetLEDMode(1, UILEDMode_FlashStartOn);
                UIControl_SetLEDMode(2, UILEDMode_FlashStartOn);
                UIControl_SetLEDMode(3, UILEDMode_FlashStartOn);
                UIControl_SetLEDMode(4, UILEDMode_FlashStartOn);
                break;

            case AppUIState_SoftApNotConnected:
                UIControl_SetLEDMode(1, UILEDMode_FlashStartOn);
                UIControl_SetLEDMode(2, UILEDMode_Off);
                UIControl_SetLEDMode(3, UILEDMode_Off);
                UIControl_SetLEDMode(4, UILEDMode_Off);
                break;

            case AppUIState_SoftApConnected:
                UIControl_SetLEDMode(1, UILEDMode_On);
                UIControl_SetLEDMode(2, UILEDMode_Off);
                UIControl_SetLEDMode(3, UILEDMode_Off);
                UIControl_SetLEDMode(4, UILEDMode_Off);
                break;

            case AppUIState_SoftApNotConfigured:
                UIControl_SetLEDMode(1, UILEDMode_On);
                UIControl_SetLEDMode(2, UILEDMode_FlashStartOn);
                UIControl_SetLEDMode(3, UILEDMode_Off);
                UIControl_SetLEDMode(4, UILEDMode_Off);
                break;

            case AppUIState_SoftApConfigured:
                UIControl_SetLEDMode(1, UILEDMode_On);
                UIControl_SetLEDMode(2, UILEDMode_On);
                UIControl_SetLEDMode(3, UILEDMode_Off);
                UIControl_SetLEDMode(4, UILEDMode_Off);
                break;

            case AppUIState_AppInitConnectingToNetwork:
                UIControl_SetLEDMode(1, UILEDMode_On);
                UIControl_SetLEDMode(2, UILEDMode_On);
                UIControl_SetLEDMode(3, UILEDMode_FlashStartOn);
                UIControl_SetLEDMode(4, UILEDMode_Off);
                break;

            case AppUIState_AppInitConnectedToNetwork:
                UIControl_SetLEDMode(1, UILEDMode_On);
                UIControl_SetLEDMode(2, UILEDMode_On);
                UIControl_SetLEDMode(3, UILEDMode_On);
                UIControl_SetLEDMode(4, UILEDMode_Off);
                break;

            case AppUIState_AppInitConnectingToServer:
                UIControl_SetLEDMode(1, UILEDMode_On);
                UIControl_SetLEDMode(2, UILEDMode_On);
                UIControl_SetLEDMode(3, UILEDMode_On);
                UIControl_SetLEDMode(4, UILEDMode_FlashStartOn);
                break;

            case AppUIState_AppConnectedToServer:
                UIControl_SetLEDMode(1, UILEDMode_On);
                UIControl_SetLEDMode(2, UILEDMode_On);
                UIControl_SetLEDMode(3, UILEDMode_On);
                UIControl_SetLEDMode(4, UILEDMode_On);
                break;

            case AppUIState_AppHardwareError:
                UIControl_SetLEDMode(1, UILEDMode_FlashStartOn);
                UIControl_SetLEDMode(2, UILEDMode_FlashStartOn);
                UIControl_SetLEDMode(3, UILEDMode_FlashStartOn);
                UIControl_SetLEDMode(4, UILEDMode_FlashStartOn);
                break;

            case AppUIState_None:
            default:
                UIControl_SetLEDMode(1, UILEDMode_Off);
                UIControl_SetLEDMode(3, UILEDMode_Off);
                UIControl_SetLEDMode(2, UILEDMode_Off);
                UIControl_SetLEDMode(4, UILEDMode_Off);
                break;
        }
        result = true;
    }
    return result;
}

void UIControl_pollInputSensors(void)
{
    // Read button inputs
    bool switch1 = (BSP_SwitchStateGet(BSP_SWITCH_1) == BSP_SWITCH_STATE_PRESSED);
    bool switch2 = (BSP_SwitchStateGet(BSP_SWITCH_2) == BSP_SWITCH_STATE_PRESSED);
    ButtonObject_Input(0, switch1);
    ButtonObject_Input(1, switch2);

    // TODO - read temperature
    TemperatureObject_Input(0, 19.4);
    // TODO - read analogue input
}

