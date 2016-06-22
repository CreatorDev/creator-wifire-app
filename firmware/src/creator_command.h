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

#ifndef _CREATOR_COMMAND_H_
#define _CREATOR_COMMAND_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef bool (*CreatorCommand_Handler)(int argc, char **argv);		// Function pointer type representing a command handler.

void CreatorCommand_Cleanup(void);
bool CreatorCommand_Initialise(void);

bool CreatorCommand_PromptUser(void);
bool CreatorCommand_PromptUserWithQuery(char *query);
int32_t CreatorCommand_ReadInputIntegerOption(uint32_t inputRange, bool zeroBased, bool obscureCharacters);
int32_t CreatorCommand_ReadInputIntegerOptionWithQuery(char *query, uint32_t inputRange, bool zeroBased, bool obscureCharacters);
int32_t CreatorCommand_ReadInputString(uint8_t* buffer, uint32_t buffSize, bool obscureCharacters);
int32_t CreatorCommand_ReadInputStringWithQuery(char *query, uint8_t* buffer, uint32_t buffSize, bool obscureCharacters);

bool CreatorCommand_RegisterCommand(char *groupName, char *name, char *description, CreatorCommand_Handler handler);
bool CreatorCommand_RegisterCommandGroup(char *groupName, char *groupDescription);

bool CreatorCommand_SetLineTerminator(char *lineTerminator);
bool CreatorCommand_Task(void);


#ifdef __cplusplus
}
#endif

#endif
