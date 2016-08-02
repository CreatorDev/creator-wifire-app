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
    AwaBoolean On;       // resource 5850
} LEDObject;

#define LED_INSTANCES (NUMBER_OF_LEDS)

static LEDObject ledObject[LED_INSTANCES];
static LEDObject prevLed[LED_INSTANCES];

void LedObject_Create(AwaStaticClient * awaClient)
{
    // Define LED objects
    AwaStaticClient_DefineObject(awaClient, IPSO_LIGHT_CONTROL_OBJECT, "Light", 0, LED_INSTANCES);
    AwaStaticClient_DefineResource(awaClient, IPSO_LIGHT_CONTROL_OBJECT, IPSO_ON_OFF, "On", AwaResourceType_Boolean, 0, 1, AwaResourceOperations_ReadWrite);
	AwaStaticClient_SetResourceStorageWithPointer(awaClient, IPSO_LIGHT_CONTROL_OBJECT, IPSO_ON_OFF, &ledObject[0].On, sizeof(ledObject[0].On), sizeof(ledObject[0]));

    // Create object instances
    int instance;
    for (instance = 0; instance < LED_INSTANCES; instance++)
    {
        AwaStaticClient_CreateObjectInstance(awaClient, IPSO_LIGHT_CONTROL_OBJECT, instance);
        AwaStaticClient_CreateResource(awaClient, IPSO_LIGHT_CONTROL_OBJECT, instance, IPSO_ON_OFF);
        ledObject[instance].On = true;
        prevLed[instance].On = true;
    }
}

void LedObject_Close(void)
{
    // Clear all leds to show shutdown
    UIControl_ClearLEDs();
}

void LedObject_Update(AwaStaticClient * awaClient)
{
    int index;
	for (index = 0; index < NUMBER_OF_LEDS; index++)
    {
		// Check if any LED changed
        if (prevLed[index].On != ledObject[index].On)
        {
            prevLed[index].On = ledObject[index].On;
            UIControl_SetLEDMode(index, UILEDMode_Manual);
            if (prevLed[index].On)
                UIControl_SetLEDState(index, UILEDState_On);
            else
                UIControl_SetLEDState(index, UILEDState_Off);

            Creator_Log(CreatorLogLevel_Info, "Set LED%d %s", index+1, prevLed[index].On ? "On" : "Off");
            AwaStaticClient_ResourceChanged(awaClient, IPSO_LIGHT_CONTROL_OBJECT, index, IPSO_ON_OFF);
        }
    }
}

void LedObject_Refresh(void)
{
    // Refresh all LEDs (e.g. to restore the settings after connection lost/recovery)
    int index;
    for (index = 0; index < NUMBER_OF_LEDS; index++)
    {
        prevLed[index].On = ledObject[index].On;
        UIControl_SetLEDMode(index, UILEDMode_Manual);
        UIControl_SetLEDState(index, prevLed[index].On ? UILEDState_On : UILEDState_Off);
    }
}

bool LedObject_Command(int ledID, bool ledOn)
{
    // LED control from user console command
    bool result = false;
    if (ledID >= 0 && ledID < NUMBER_OF_LEDS)
    {
        // Change LED in the next update
        ledObject[ledID].On = ledOn;
        result = true;
    }
    return result;
}
