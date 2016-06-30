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

/*! \file led_object.c
 *  \brief LED Objects - support IPSO Light objects to control WiFire LEDs
 */

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "creator/core/creator_debug.h"
#include "ipso_object_definitions.h"
#include "ui_control.h"
#include "led_object.h"

typedef struct      // IPSO object: 3311 - Light control
{
    AwaBoolean OnOff;       // resource 5850
} LEDObject;

#define LED_INSTANCES 4

static LEDObject leds[LED_INSTANCES];
static LEDObject prevLeds[LED_INSTANCES];

void LedObject_Create(AwaStaticClient * awaClient)
{
    // Define LED objects
    AwaStaticClient_DefineObject(awaClient, IPSO_LIGHT_CONTROL_OBJECT, "Light", 0, LED_INSTANCES);
    AwaStaticClient_DefineResource(awaClient, IPSO_LIGHT_CONTROL_OBJECT, IPSO_ON_OFF, "OnOff", AwaResourceType_Boolean, 0, 1, AwaResourceOperations_ReadWrite);
	AwaStaticClient_SetResourceStorageWithPointer(awaClient, IPSO_LIGHT_CONTROL_OBJECT, IPSO_ON_OFF, &leds[0].OnOff, sizeof(leds[0].OnOff), sizeof(leds[0]));

    // Create object instances
    int instance;
    for (instance = 0; instance < LED_INSTANCES; instance++)
    {
        AwaStaticClient_CreateObjectInstance(awaClient, IPSO_LIGHT_CONTROL_OBJECT, instance);
        AwaStaticClient_CreateResource(awaClient, IPSO_LIGHT_CONTROL_OBJECT, instance, IPSO_ON_OFF);
        leds[instance].OnOff = true;
        prevLeds[instance].OnOff = true;
    }
}

void LedObject_Close(void)
{
    // TODO - set default LED pattern...
}

void LedObject_Update(AwaStaticClient * awaClient)
{
    int index;
	for (index = 0; index < MAX_LEDS; index++)
    {
		// Check if any LED changed
        if (prevLeds[index].OnOff != leds[index].OnOff)
        {
            prevLeds[index].OnOff = leds[index].OnOff;
            UIControl_SetLEDMode(index + 1, UILEDMode_Manual);
            if (prevLeds[index].OnOff)
                UIControl_SetLEDState(index + 1, UILEDState_On);
            else
                UIControl_SetLEDState(index + 1, UILEDState_Off);

            Creator_Log(CreatorLogLevel_Info, "Set LED%d %s", index + 1, prevLeds[index].OnOff ? "On" : "Off");
            AwaStaticClient_ResourceChanged(awaClient, IPSO_LIGHT_CONTROL_OBJECT, index, IPSO_ON_OFF);
        }
    }
}
