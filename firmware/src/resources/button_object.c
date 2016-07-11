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

/*! \file button_object.c
 *  \brief Button Objects - support IPSO digital input objects to detect and notify button presses
 */

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "creator/core/creator_debug.h"
#include "creator/core/creator_timer.h"
#include "ipso_object_definitions.h"
#include "button_object.h"

typedef struct          // IPSO object: 3311 - Digital input
{
    AwaBoolean State;           // resource 5500
    AwaInteger Counter;         // resource 5501
    AwaInteger DebouncePeriod;  // resource 5503
} ButtonObject;

typedef struct
{
    bool CurrentState;
    bool DebouncedState;
    uint StartTime;     // system ticks (ms)
} ButtonInput;

#define BUTTON_INSTANCES 2
#define DEFAULT_DEBOUNCE_TIME_MS 100

static ButtonObject buttonObject[BUTTON_INSTANCES];
static ButtonInput buttonInput[BUTTON_INSTANCES];

void ButtonObject_Create(AwaStaticClient * awaClient)
{
    // Define Button objects
    AwaStaticClient_DefineObject(awaClient, IPSO_DIGITAL_INPUT_OBJECT, "Button", 0, BUTTON_INSTANCES);
    AwaStaticClient_DefineResource(awaClient, IPSO_DIGITAL_INPUT_OBJECT, IPSO_DIGITAL_INPUT_STATE, "State", AwaResourceType_Boolean, 0, 1, AwaResourceOperations_ReadOnly);
	AwaStaticClient_SetResourceStorageWithPointer(awaClient, IPSO_DIGITAL_INPUT_OBJECT, IPSO_DIGITAL_INPUT_STATE, &buttonObject[0].State, sizeof(buttonObject[0].State), sizeof(buttonObject[0]));
    AwaStaticClient_DefineResource(awaClient, IPSO_DIGITAL_INPUT_OBJECT, IPSO_DIGITAL_INPUT_COUNTER, "Counter", AwaResourceType_Integer, 0, 1, AwaResourceOperations_ReadOnly);
	AwaStaticClient_SetResourceStorageWithPointer(awaClient, IPSO_DIGITAL_INPUT_OBJECT, IPSO_DIGITAL_INPUT_COUNTER, &buttonObject[0].Counter, sizeof(buttonObject[0].Counter), sizeof(buttonObject[0]));
    AwaStaticClient_DefineResource(awaClient, IPSO_DIGITAL_INPUT_OBJECT, IPSO_DIGITAL_INPUT_DEBOUNCE_PERIOD, "Debounce", AwaResourceType_Integer, 0, 1, AwaResourceOperations_ReadWrite);
	AwaStaticClient_SetResourceStorageWithPointer(awaClient, IPSO_DIGITAL_INPUT_OBJECT, IPSO_DIGITAL_INPUT_DEBOUNCE_PERIOD, &buttonObject[0].DebouncePeriod, sizeof(buttonObject[0].DebouncePeriod), sizeof(buttonObject[0]));

    // Create object instances
    memset(&buttonObject, 0, sizeof(buttonObject));
    memset(&buttonInput, 0, sizeof(buttonInput));
    int instance;
    for (instance = 0; instance < BUTTON_INSTANCES; instance++)
    {
        AwaStaticClient_CreateObjectInstance(awaClient, IPSO_DIGITAL_INPUT_OBJECT, instance);
        AwaStaticClient_CreateResource(awaClient, IPSO_DIGITAL_INPUT_OBJECT, instance, IPSO_DIGITAL_INPUT_STATE);
        AwaStaticClient_CreateResource(awaClient, IPSO_DIGITAL_INPUT_OBJECT, instance, IPSO_DIGITAL_INPUT_COUNTER);
        AwaStaticClient_CreateResource(awaClient, IPSO_DIGITAL_INPUT_OBJECT, instance, IPSO_DIGITAL_INPUT_DEBOUNCE_PERIOD);
        buttonObject[instance].DebouncePeriod = DEFAULT_DEBOUNCE_TIME_MS;
    }
}

void ButtonObject_Update(AwaStaticClient * awaClient)
{
    int index;
	for (index = 0; index < BUTTON_INSTANCES; index++)
    {
		// Check if any Button changed
        if (buttonObject[index].State != buttonInput[index].DebouncedState)
        {
            buttonObject[index].State = buttonInput[index].DebouncedState;
            if (buttonObject[index].State)
                buttonObject[index].Counter++;
            Creator_Log(CreatorLogLevel_Debug, "Button%d %s", index + 1, buttonObject[index].State ? "On" : "Off");
            AwaStaticClient_ResourceChanged(awaClient, IPSO_DIGITAL_INPUT_OBJECT, index, IPSO_DIGITAL_INPUT_STATE);
        }
    }
}

void ButtonObject_Input(int buttonID, bool inputState)
{
    if (buttonID < BUTTON_INSTANCES)
    {
        if (buttonInput[buttonID].CurrentState != inputState)
        {
            buttonInput[buttonID].StartTime = CreatorTimer_GetTickCount();
            buttonInput[buttonID].CurrentState = inputState;
        }
        if (buttonInput[buttonID].DebouncedState != inputState)
        {
            // Debounce changes
            if ((CreatorTimer_GetTickCount() - buttonInput[buttonID].StartTime) >= buttonObject[buttonID].DebouncePeriod)
            {
                buttonInput[buttonID].DebouncedState = inputState;
            }
        }
    }
}

