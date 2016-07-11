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

/*! \file analog_input_object.c
 *  \brief Analog Input Object - support IPSO analog input object to read WiFire potentiometer
 */

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "creator/core/creator_debug.h"
#include "creator/core/creator_timer.h"
#include "ipso_object_definitions.h"
#include "analog_input_object.h"

#define SENSORTYPE_SIZE  20
#define SENSORTYPE_POTENTIOMETER   "Potentiometer"

typedef struct          // IPSO object: 3202 - AnalogInput
{
    AwaFloat Value;                     // resource 5600
    AwaFloat Min;                       // resource 5601
    AwaFloat Max;                       // resource 5602
    char SensorType[SENSORTYPE_SIZE];   // resource 5751
    bool IsInitialised;
} AnalogInputObject;

typedef struct
{
    float Value;
} AnalogInput;

#define ANALOGINPUT_INSTANCES 1

static AnalogInputObject analogInputObject[ANALOGINPUT_INSTANCES];
static AnalogInput analogInput[ANALOGINPUT_INSTANCES];

void AnalogInputObject_Create(AwaStaticClient * awaClient)
{
    // Define AnalogInput objects
    AwaStaticClient_DefineObject(awaClient, IPSO_ANALOGUE_INPUT_OBJECT, "AnalogInput", 0, ANALOGINPUT_INSTANCES);
    AwaStaticClient_DefineResource(awaClient, IPSO_ANALOGUE_INPUT_OBJECT, IPSO_ANALOGUE_INPUT_CURRENT_VALUE, "Value", AwaResourceType_Float, 0, 1, AwaResourceOperations_ReadOnly);
	AwaStaticClient_SetResourceStorageWithPointer(awaClient, IPSO_ANALOGUE_INPUT_OBJECT, IPSO_ANALOGUE_INPUT_CURRENT_VALUE, &analogInputObject[0].Value, sizeof(analogInputObject[0].Value), sizeof(analogInputObject[0]));
    AwaStaticClient_DefineResource(awaClient, IPSO_ANALOGUE_INPUT_OBJECT, IPSO_MIN_MEASURED_VALUE, "Min", AwaResourceType_Float, 0, 1, AwaResourceOperations_ReadOnly);
	AwaStaticClient_SetResourceStorageWithPointer(awaClient, IPSO_ANALOGUE_INPUT_OBJECT, IPSO_MIN_MEASURED_VALUE, &analogInputObject[0].Min, sizeof(analogInputObject[0].Min), sizeof(analogInputObject[0]));
    AwaStaticClient_DefineResource(awaClient, IPSO_ANALOGUE_INPUT_OBJECT, IPSO_MAX_MEASURED_VALUE, "Max", AwaResourceType_Float, 0, 1, AwaResourceOperations_ReadOnly);
	AwaStaticClient_SetResourceStorageWithPointer(awaClient, IPSO_ANALOGUE_INPUT_OBJECT, IPSO_MAX_MEASURED_VALUE, &analogInputObject[0].Max, sizeof(analogInputObject[0].Max), sizeof(analogInputObject[0]));
    AwaStaticClient_DefineResource(awaClient, IPSO_ANALOGUE_INPUT_OBJECT, IPSO_SENSOR_TYPE, "SensorType", AwaResourceType_String, 0, 1, AwaResourceOperations_ReadOnly);
	AwaStaticClient_SetResourceStorageWithPointer(awaClient, IPSO_ANALOGUE_INPUT_OBJECT, IPSO_SENSOR_TYPE, &analogInputObject[0].SensorType, sizeof(analogInputObject[0].SensorType), sizeof(analogInputObject[0]));

    // Create object instances
    memset(&analogInputObject, 0, sizeof(analogInputObject));
    memset(&analogInput, 0, sizeof(analogInput));
    int instance;
    for (instance = 0; instance < ANALOGINPUT_INSTANCES; instance++)
    {
        AwaStaticClient_CreateObjectInstance(awaClient, IPSO_ANALOGUE_INPUT_OBJECT, instance);
        AwaStaticClient_CreateResource(awaClient, IPSO_ANALOGUE_INPUT_OBJECT, instance, IPSO_ANALOGUE_INPUT_CURRENT_VALUE);
        AwaStaticClient_CreateResource(awaClient, IPSO_ANALOGUE_INPUT_OBJECT, instance, IPSO_MIN_MEASURED_VALUE);
        AwaStaticClient_CreateResource(awaClient, IPSO_ANALOGUE_INPUT_OBJECT, instance, IPSO_MAX_MEASURED_VALUE);
        AwaStaticClient_CreateResource(awaClient, IPSO_ANALOGUE_INPUT_OBJECT, instance, IPSO_SENSOR_TYPE);
        strncpy(analogInputObject[instance].SensorType, SENSORTYPE_POTENTIOMETER, SENSORTYPE_SIZE);
    }
}

void AnalogInputObject_Update(AwaStaticClient * awaClient)
{
    int index;
	for (index = 0; index < ANALOGINPUT_INSTANCES; index++)
    {
		// Check if input changed
        AwaFloat value = analogInput[index].Value;
        if (analogInputObject[index].Value != value)
        {
            analogInputObject[index].Value = value;
            if (analogInputObject[index].IsInitialised)
            {
                if (value < analogInputObject[index].Min)
                    analogInputObject[index].Min = value;
                if (value > analogInputObject[index].Max)
                    analogInputObject[index].Max = value;
            }
            else
            {
                analogInputObject[index].IsInitialised = true;
                analogInputObject[index].Min = value;
                analogInputObject[index].Max = value;
            }
            AwaStaticClient_ResourceChanged(awaClient, IPSO_ANALOGUE_INPUT_OBJECT, index, IPSO_ANALOGUE_INPUT_CURRENT_VALUE);
        }
    }
}

void AnalogInputObject_Input(int analogInputID, float value)
{
    if (analogInputID < ANALOGINPUT_INSTANCES)
    {
        analogInput[analogInputID].Value = value;
    }
}

