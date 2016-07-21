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

/*! \file execute_command_object.c
 *  \brief ExecutCommand Object - support ExecuteCommand object for WiFire specific console commands
 */

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "creator/core/creator_debug.h"
#include "creator/core/creator_timer.h"
#include "execute_command_object.h"

#define EXECUTECOMMAND_OBJECT   20001        // Execute command object id

// Resource IDs
#define EXECUTECOMMAND_VALUE    0
#define EXECUTECOMMAND_COUNT  1

#define EXECUTECOMMAND_INSTANCES 1

#define COMMANDVALUE_LEN 100


typedef struct
{
    char Value[COMMANDVALUE_LEN];    // resource 0
    AwaInteger Count;                     // resource 1
} ExecutCommandObject;

static ExecutCommandObject executeCommandObject[EXECUTECOMMAND_INSTANCES];


bool SetCommandValue(char *value, int valueLength, const char *dataPointer, size_t dataSize)
{
    bool result = false;
    memset(value, 0, valueLength);
    if (dataSize < valueLength)
    {
        memset(value, 0, valueLength);
        memcpy(value, dataPointer, dataSize);
        result = true;
    }
    return result;
}

AwaResult handler(AwaStaticClient * client, AwaOperation operation, AwaObjectID objectID,
            AwaObjectInstanceID objectInstanceID, AwaResourceID resourceID, AwaResourceInstanceID resourceInstanceID,
            void ** dataPointer, size_t * dataSize, bool * changed)
{
    AwaResult result = AwaResult_InternalError;
    if ((objectID == EXECUTECOMMAND_OBJECT) && (objectInstanceID >= 0) && (objectInstanceID < EXECUTECOMMAND_INSTANCES))
    {
        switch (operation)
        {
            case AwaOperation_CreateObjectInstance:
            {
                result = AwaResult_SuccessCreated;
                memset(&executeCommandObject[objectInstanceID], 0, sizeof(executeCommandObject[objectInstanceID]));
                break;
            }
            case AwaOperation_CreateResource:
            {
                switch (resourceID)
                {
                    case EXECUTECOMMAND_VALUE:
                        memset(executeCommandObject[objectInstanceID].Value, 0, COMMANDVALUE_LEN);
                        break;
                    default:
                        break;
                }
                result = AwaResult_SuccessCreated;
                break;
            }
            case AwaOperation_Write:
            {
                result = AwaResult_BadRequest;
                switch (resourceID)
                {
                    case EXECUTECOMMAND_VALUE:
                    {
                        if (SetCommandValue(executeCommandObject[objectInstanceID].Value, COMMANDVALUE_LEN, *dataPointer, *dataSize))
                        {
                            const char *command = executeCommandObject[objectInstanceID].Value;
                            //Creator_Log(CreatorLogLevel_Debug, "ExecuteCommand command: %s", command);
                            if (CreatorCommand_ExecuteCommand(command, true))
                            {
                                result = AwaResult_SuccessChanged;
                            }   
                            executeCommandObject[objectInstanceID].Count++;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case AwaOperation_Read:
            {
                result = AwaResult_BadRequest;
                switch (resourceID)
                {
                    case EXECUTECOMMAND_VALUE:
                    {
                        *dataPointer = executeCommandObject[objectInstanceID].Value;
                        *dataSize = strlen(executeCommandObject[objectInstanceID].Value);
                        result = AwaResult_SuccessContent;
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }
    }
    return result;
}
void ExecuteCommandObject_Create(AwaStaticClient * awaClient)
{
    // Define ExecuteCommand object with handler support for execute command values
    AwaStaticClient_DefineObject(awaClient, EXECUTECOMMAND_OBJECT, "ExecuteCommand", 0, EXECUTECOMMAND_INSTANCES);
    AwaStaticClient_SetObjectOperationHandler(awaClient, EXECUTECOMMAND_OBJECT, handler);
    AwaStaticClient_DefineResource(awaClient, EXECUTECOMMAND_OBJECT, EXECUTECOMMAND_VALUE, "CommandValue", AwaResourceType_String, 0, 1, AwaResourceOperations_ReadWrite);
    AwaStaticClient_SetResourceOperationHandler(awaClient, EXECUTECOMMAND_OBJECT, EXECUTECOMMAND_VALUE, handler);
    AwaStaticClient_DefineResource(awaClient, EXECUTECOMMAND_OBJECT, EXECUTECOMMAND_COUNT, "Counter", AwaResourceType_Integer, 0, 1, AwaResourceOperations_ReadOnly);
	AwaStaticClient_SetResourceStorageWithPointer(awaClient, EXECUTECOMMAND_OBJECT, EXECUTECOMMAND_COUNT, &executeCommandObject[0].Count, sizeof(executeCommandObject[0].Count), sizeof(executeCommandObject[0]));
    
    // Create object instance for execute command handler
    int instance = 0;
    AwaStaticClient_CreateObjectInstance(awaClient, EXECUTECOMMAND_OBJECT, instance);
    AwaStaticClient_CreateResource(awaClient, EXECUTECOMMAND_OBJECT, instance, EXECUTECOMMAND_VALUE);
    AwaStaticClient_CreateResource(awaClient, EXECUTECOMMAND_OBJECT, instance, EXECUTECOMMAND_COUNT);

}

void ExecuteCommandObject_ResetStatistics(void)
{
    int index;
	for (index = 0; index < EXECUTECOMMAND_INSTANCES; index++)
    {
        executeCommandObject[index].Count = 0;
    }
}

