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

/*! \file temperature_object.c
 *  \brief Temperature Object - support IPSO temperature object to read WiFire temperature
 */

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "creator/core/creator_debug.h"
#include "creator/core/creator_timer.h"
#include "ipso_object_definitions.h"
#include "temperature_object.h"

#define TEMPERATURE_UNITS_SIZE  5
#define TEMPERATURE_UNITS_CELSIUS   "Cel"
#define TEMPERATURE_UNITS_FARENHEIT "Far"

typedef struct          // IPSO object: 3303 - Temperature
{
    AwaFloat Value;                     // resource 5700
    char Units[TEMPERATURE_UNITS_SIZE]; // resource 5701
    AwaFloat Min;                       // resource 5601
    AwaFloat Max;                       // resource 5602
    bool IsInitialised;
} TemperatureObject;

#define TEMPERATURE_AVERAGE_COUNT 20
typedef struct
{
    float Value;
    int AverageCount;
    float AverageTotal;
} TemperatureInput;

#define TEMPERATURE_INSTANCES 1

static TemperatureObject temperatureObject[TEMPERATURE_INSTANCES];
static TemperatureInput temperatureInput[TEMPERATURE_INSTANCES];

void TemperatureObject_Create(AwaStaticClient * awaClient)
{
    // Define Temperature objects
    AwaStaticClient_DefineObject(awaClient, IPSO_TEMPERATURE_OBJECT, "Temperature", 0, TEMPERATURE_INSTANCES);
    AwaStaticClient_DefineResource(awaClient, IPSO_TEMPERATURE_OBJECT, IPSO_SENSOR_VALUE, "Value", AwaResourceType_Float, 0, 1, AwaResourceOperations_ReadOnly);
	AwaStaticClient_SetResourceStorageWithPointer(awaClient, IPSO_TEMPERATURE_OBJECT, IPSO_SENSOR_VALUE, &temperatureObject[0].Value, sizeof(temperatureObject[0].Value), sizeof(temperatureObject[0]));
    AwaStaticClient_DefineResource(awaClient, IPSO_TEMPERATURE_OBJECT, IPSO_UNITS, "Units", AwaResourceType_String, 0, 1, AwaResourceOperations_ReadOnly);
	AwaStaticClient_SetResourceStorageWithPointer(awaClient, IPSO_TEMPERATURE_OBJECT, IPSO_UNITS, &temperatureObject[0].Units, sizeof(temperatureObject[0].Units), sizeof(temperatureObject[0]));
    AwaStaticClient_DefineResource(awaClient, IPSO_TEMPERATURE_OBJECT, IPSO_MIN_MEASURED_VALUE, "Min", AwaResourceType_Float, 0, 1, AwaResourceOperations_ReadOnly);
	AwaStaticClient_SetResourceStorageWithPointer(awaClient, IPSO_TEMPERATURE_OBJECT, IPSO_MIN_MEASURED_VALUE, &temperatureObject[0].Min, sizeof(temperatureObject[0].Min), sizeof(temperatureObject[0]));
    AwaStaticClient_DefineResource(awaClient, IPSO_TEMPERATURE_OBJECT, IPSO_MAX_MEASURED_VALUE, "Max", AwaResourceType_Float, 0, 1, AwaResourceOperations_ReadOnly);
	AwaStaticClient_SetResourceStorageWithPointer(awaClient, IPSO_TEMPERATURE_OBJECT, IPSO_MAX_MEASURED_VALUE, &temperatureObject[0].Max, sizeof(temperatureObject[0].Max), sizeof(temperatureObject[0]));

    // Create object instances
    memset(&temperatureObject, 0, sizeof(temperatureObject));
    memset(&temperatureInput, 0, sizeof(temperatureInput));
    int instance;
    for (instance = 0; instance < TEMPERATURE_INSTANCES; instance++)
    {
        AwaStaticClient_CreateObjectInstance(awaClient, IPSO_TEMPERATURE_OBJECT, instance);
        AwaStaticClient_CreateResource(awaClient, IPSO_TEMPERATURE_OBJECT, instance, IPSO_SENSOR_VALUE);
        AwaStaticClient_CreateResource(awaClient, IPSO_TEMPERATURE_OBJECT, instance, IPSO_UNITS);
        AwaStaticClient_CreateResource(awaClient, IPSO_TEMPERATURE_OBJECT, instance, IPSO_MIN_MEASURED_VALUE);
        AwaStaticClient_CreateResource(awaClient, IPSO_TEMPERATURE_OBJECT, instance, IPSO_MAX_MEASURED_VALUE);
        strncpy(temperatureObject[instance].Units, TEMPERATURE_UNITS_CELSIUS, TEMPERATURE_UNITS_SIZE);
    }
}

void TemperatureObject_Update(AwaStaticClient * awaClient)
{
    int index;
	for (index = 0; index < TEMPERATURE_INSTANCES; index++)
    {
		// Check if temperature changed
        AwaFloat value = temperatureInput[index].Value;
        if (temperatureObject[index].Value != value)
        {
            temperatureObject[index].Value = value;
            if (temperatureObject[index].IsInitialised)
            {
                if (value < temperatureObject[index].Min)
                    temperatureObject[index].Min = value;
                if (value > temperatureObject[index].Max)
                    temperatureObject[index].Max = value;
            }
            else
            {
                //Creator_Log(CreatorLogLevel_Info, "Temperature %d", (int)value);    // TODO - remove (note %lf doesn't work!))
                temperatureObject[index].IsInitialised = true;
                temperatureObject[index].Min = value;
                temperatureObject[index].Max = value;
            }
            //AwaStaticClient_ObjectInstanceChanged(awaClient, IPSO_TEMPERATURE_OBJECT, index); // TODO - missing from static API ?
            AwaStaticClient_ResourceChanged(awaClient, IPSO_TEMPERATURE_OBJECT, index, IPSO_SENSOR_VALUE);
        }
    }
}

void TemperatureObject_Input(int temperatureID, float value)
{
    if (temperatureID < TEMPERATURE_INSTANCES)
    {
#if TEMPERATURE_AVERAGE_COUNT
        // Update average temperature (ADC values read have a lot of jitter)
        TemperatureInput *input = &temperatureInput[temperatureID];
        input->AverageTotal += value;
        if (++input->AverageCount >= TEMPERATURE_AVERAGE_COUNT)
        {
            float average = input->AverageTotal / TEMPERATURE_AVERAGE_COUNT;
            input->Value = average;
            input->AverageTotal = 0;
            input->AverageCount = 0;
        }
#else
        temperatureInput[temperatureID].Value = value;
#endif
    }
}

void TemperatureObject_ResetStatistics(void)
{
    int index;
    for (index = 0; index < TEMPERATURE_INSTANCES; index++)
    {
        TemperatureObject * temperature = &temperatureObject[index];
        if (temperature->IsInitialised)
        {
            temperature->Min = temperature->Value;
            temperature->Max = temperature->Value;
        }
    }
}

