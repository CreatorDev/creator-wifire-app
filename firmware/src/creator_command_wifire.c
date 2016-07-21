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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "creator_command.h"
#include "creator/creator_console.h"
#include "creator/core/creator_list.h"
#include "creator/core/creator_memalloc.h"


//////////////////////
// Type definitions //
//////////////////////


// An individual command
typedef struct
{
    char *name;
    char *description;
    CreatorCommand_Handler handler;
    bool allowRemoteCommand;

} Command;

// A group of commands (identified by group name)
typedef struct
{
    char *name;
    char *description;
    CreatorList commands;

} CommandGroup;



///////////////////////
// Macro definitions //
///////////////////////

#define INITIAL_COMMAND_GROUP_CAPACITY 			5		// Initial number of command groups
#define INITIAL_COMMAND_CAPACITY 				5		// Initial number of commands in a group
#define MAX_COMMAND_GROUP_NAME_LENGTH			32		// Maximum length of a command group name
#define MAX_COMMAND_GROUP_DESCRIPTION_LENGTH	128		// Maximum length of a command group description
#define MAX_COMMAND_NAME_LENGTH					32		// Maximum length of a command name
#define MAX_COMMAND_DESCRIPTION_LENGTH			128		// Maximum length of a command description
#define COMMAND_HISTORY_LENGTH					10		// Number of commands to remember in command history
#define CREATOR_COMMAND_MAX_LENGTH					64		// Maximum length of a valid command

#define CurrentBufferSize (commandBufferCursor - commandBufferStart)

#define CHAR_HISTORY_LEN 						3		// Characters to store history for non-alphanumeric key presses

#define PROMPT_CHARACTER 						'$'		// Character at prompt
#define DEFAULT_LINE_TERMINATOR					"\r\n"

#define UP_KEY 									'A'
#define DOWN_KEY								'B'
#define BACKSPACE_KEY							127




//////////////////////
// Static variables //
//////////////////////

// Module housekeeping
static bool commandModuleInitialised = false;
static bool commandModuleQuit = false;

// Registered command groups
static CreatorList commandGroups = NULL;

// Command history
static CreatorList commandHistory = NULL;
static int commandHistoryCursorPosition = 0;

// Incoming command buffer
static char commandBuffer[CREATOR_COMMAND_MAX_LENGTH + 1] = {0};
static char* commandBufferStart = commandBuffer; 				// Start of command buffer
static char* commandBufferCursor = commandBuffer; 				// Location to insert next character

//Configuration
static char* lineTerminator = NULL;



/////////////////////////
// Function Prototypes //
/////////////////////////

// Incoming Command buffer
static void CreatorCommand_ClearCommandBuffer(void);

// Command History
static bool CreatorCommand_AddCommandToHistory(char* command);
static char *CreatorCommand_GetCommandAtHistoryIndex(int index);
static int CreatorCommand_GetCommandHistoryCursorIndex(void);
static void CreatorCommand_RollBackThroughCommandHistory(void);
static void CreatorCommand_RollForwardThroughCommandHistory(void);

// Registered commands / command group management
static CommandGroup* CreatorCommand_CreateNewGroup_Internal(char *groupName, char *groupDescription);
static Command *CreatorCommand_FindCommand(CommandGroup *group, char *commandName);
static CommandGroup *CreatorCommand_FindCommandGroup(char* name);

// Function Handlers
static bool CreatorCommand_DisplayHelp(int argc, char **argv);

// Command Execution
static bool CreatorCommand_FreeArguments(int argc, char *argv[]);
static char **CreatorCommand_ParseArguments(char *command, int *argc);
static bool CreatorCommand_PrintCommandHistory(int argc, char **argv);

// Helpers
static void CreatorCommand_DisplayPrompt(void);
static bool CreatorCommand_IsWhiteSpace(char data);
static void CreatorCommand_PrintLineTerminators(int count);
static bool CreatorCommand_ValidCharacter(char data);



////////////////////
// Implementation //
////////////////////


static bool CreatorCommand_AddCommandToHistory(char* command)
{
    bool result = false;

    if (commandHistory != NULL && command != NULL)
    {
        unsigned int historyCount = CreatorList_GetCount(commandHistory);

        char *commandBuf = Creator_MemAlloc(strlen(command) + 1);
        if (commandBuf != NULL)
        {
            memcpy(commandBuf, command, strlen(command) + 1);
            result = CreatorList_Add(commandHistory, commandBuf);

            historyCount = CreatorList_GetCount(commandHistory);
            if (historyCount >= COMMAND_HISTORY_LENGTH)
            {
                char *outgoingCommand = CreatorList_GetItem(commandHistory, 0);
                CreatorList_RemoveAt(commandHistory, 0);
                if (outgoingCommand != NULL)
                {
                    Creator_MemFree((void**) &outgoingCommand);
                }
            }

            commandHistoryCursorPosition = CreatorList_GetCount(commandHistory);
        }
    }
    return result;
}

void CreatorCommand_Cleanup(void)
{
    if (!commandModuleInitialised)
        return;

    if (commandHistory != NULL)
        CreatorList_Free(&commandHistory, true);

    if (lineTerminator != NULL)
        Creator_MemFree((void**) &lineTerminator);

    if (commandGroups != NULL)
    {
        unsigned int groupCount = CreatorList_GetCount(commandGroups);
        int index = 0;
        for (index = groupCount - 1; index >= 0; index--)
        {
            // Free the name and command group
            CommandGroup *groupPtr = CreatorList_GetItem(commandGroups, index);
            if (groupPtr != NULL)
            {
                if (groupPtr->name != NULL)
                    Creator_MemFree((void*) &groupPtr->name);

                if (groupPtr->description != NULL)
                    Creator_MemFree((void*) &groupPtr->description);

                if (groupPtr->commands != NULL)
                {
                    unsigned int commandCount = CreatorList_GetCount(groupPtr->commands);
                    unsigned int index = 0;
                    for (index = 0; index < commandCount; index++)
                    {
                        Command* command = (Command *) CreatorList_GetItem(groupPtr->commands, index);
                        if (command != NULL)
                        {
                            if (command->name != NULL)
                                Creator_MemFree((void**) &command->name);

                            if (command->description != NULL)
                                Creator_MemFree((void**) &command->description);
                        }
                    }
                    CreatorList_Free(&groupPtr->commands, true);
                }
            }
        }
        CreatorList_Free(&commandGroups, true);
        commandModuleInitialised = false;
    }
}

static void CreatorCommand_ClearCommandBuffer(void)
{
    memset(commandBuffer, 0, CREATOR_COMMAND_MAX_LENGTH + 1);
    commandBufferStart = commandBuffer;
    commandBufferCursor = commandBuffer;
}

static CommandGroup* CreatorCommand_CreateNewGroup_Internal(char *groupName, char *groupDescription)
{
    CommandGroup *result = NULL;
    bool error = true;

    if (groupName != NULL && strlen(groupName) > MAX_COMMAND_GROUP_NAME_LENGTH)
        return NULL;

    CommandGroup *newCommandGroup = Creator_MemAlloc(sizeof(CommandGroup));
    if (newCommandGroup != NULL)
    {
        memset(newCommandGroup, 0, sizeof(CommandGroup));

        if (groupName != NULL)
        {
            newCommandGroup->name = Creator_MemAlloc(strlen(groupName) + 1);
            if (newCommandGroup->name != NULL)
            {
                memcpy(newCommandGroup->name, groupName, strlen(groupName) + 1);
            }
        }

        if (groupDescription != NULL)
        {
            newCommandGroup->description = Creator_MemAlloc(strlen(groupDescription) + 1);
            if (newCommandGroup->description != NULL)
            {
                memcpy(newCommandGroup->description, groupDescription, strlen(groupDescription) + 1);

                CreatorList newCommands = CreatorList_New(INITIAL_COMMAND_CAPACITY);
                if (newCommands != NULL)
                {
                    newCommandGroup->commands = newCommands;
                    if (CreatorList_Add(commandGroups, (void*) newCommandGroup))
                    {
                        result = newCommandGroup;
                        error = false;
                    }
                }
            }
        }
    }

    // Cleanup in event of failure to create a new group
    if (error)
    {
        if (newCommandGroup->commands != NULL)
            CreatorList_Free(&newCommandGroup->commands, true);

        if (newCommandGroup->description != NULL)
            Creator_MemFree((void**) &newCommandGroup->description);

        if (newCommandGroup->name != NULL)
            Creator_MemFree((void**) &newCommandGroup->name);

        if (newCommandGroup != NULL)
            Creator_MemFree((void**) &newCommandGroup);
    }

    return result;
}

static bool CreatorCommand_DisplayHelp(int argc, char **argv)
{
    if (commandGroups == NULL)
        return false;

    unsigned int groupCount = CreatorList_GetCount(commandGroups);

    if (argc == 1)
    {
        if (groupCount > 1)
        {
            CreatorCommand_PrintLineTerminators(2);
            CreatorConsole_Printf("------- Supported command groups ------");
            CreatorCommand_PrintLineTerminators(1);
        }

        CommandGroup *ungroupedCommandGroupPtr = NULL;
        int index = 0;
        for (index = 0; index < groupCount; index++)
        {
            // Free the name and command group
            CommandGroup *groupPtr = CreatorList_GetItem(commandGroups, index);
            if (groupPtr != NULL)
            {
                if (groupPtr->name != NULL)
                {
                    CreatorConsole_Printf(" *** %s: %s ***", groupPtr->name, groupPtr->description);
                    CreatorCommand_PrintLineTerminators(1);
                }
                else
                {
                    ungroupedCommandGroupPtr = groupPtr;
                }
            }
        }

        if (ungroupedCommandGroupPtr != NULL)
        {
            CreatorConsole_Printf("---------- Built in commands ----------");
            CreatorCommand_PrintLineTerminators(1);
            unsigned int ungroupedCommandCount = CreatorList_GetCount(ungroupedCommandGroupPtr->commands);
            unsigned int index = 0;
            for (index = 0; index < ungroupedCommandCount; index++)
            {
                Command* command = (Command *) CreatorList_GetItem(ungroupedCommandGroupPtr->commands, index);
                if (command != NULL)
                {
                    CreatorConsole_Printf(" *** %s: %s ***", command->name, command->description);
                    CreatorCommand_PrintLineTerminators(1);
                }
            }
        }
        CreatorCommand_PrintLineTerminators(2);
    }
    else if (argc == 2)
    {
        CommandGroup *cmdGroup = CreatorCommand_FindCommandGroup(argv[1]);
        if (cmdGroup != NULL)
        {
            unsigned int commandCount = CreatorList_GetCount(cmdGroup->commands);
            unsigned int index = 0;
            for (index = 0; index < commandCount; index++)
            {
                Command* command = (Command *) CreatorList_GetItem(cmdGroup->commands, index);
                if (command != NULL)
                {
                    CreatorConsole_Printf(" *** %s: %s ***", command->name, command->description);
                    CreatorCommand_PrintLineTerminators(1);
                }
            }
            CreatorCommand_PrintLineTerminators(1);
        }
        else
        {
            CreatorConsole_Printf("Unknown command group. See the 'help' command for a list of valid command groups.");
            CreatorCommand_PrintLineTerminators(1);
        }
    }
    else
    {
        CreatorConsole_Printf("Invalid number of arguments (found %d) to help command.", argc - 1);
        CreatorCommand_PrintLineTerminators(1);
        CreatorConsole_Printf("Usage: help <optional group name>");
        CreatorCommand_PrintLineTerminators(2);
    }

    return true;
}

static void CreatorCommand_DisplayPrompt(void)
{
    CreatorConsole_Printf("%c ", PROMPT_CHARACTER);
}

bool CreatorCommand_ExecuteCommand(char *command, bool isRemoteCommand)
{
    bool result = false;
    int argc = 0;
    char **argv = CreatorCommand_ParseArguments(command, &argc);
    if (argv != NULL && argc > 0)
    {
        // Find command in all groups
        unsigned int groupCount = CreatorList_GetCount(commandGroups);
        int index = 0;
        for (index = 0; index < groupCount; index++)
        {
            CommandGroup *groupPtr = CreatorList_GetItem(commandGroups, index);
            if (groupPtr != NULL && argv[0] != NULL)
            {
                Command *commandInfo = CreatorCommand_FindCommand(groupPtr, argv[0]);
                if (commandInfo != NULL)
                {
                    if (commandInfo->handler != NULL && (!isRemoteCommand || commandInfo->allowRemoteCommand))
                    {
                        commandInfo->handler(argc, argv);
                        result = true;
                    }
                    break;
                }
            }
        }
    }
    CreatorCommand_FreeArguments(argc, argv);
    return result;
}

static Command *CreatorCommand_FindCommand(CommandGroup *group, char *commandName)
{
    Command *result = NULL;

    if (group == NULL || group->commands == NULL || commandName == NULL)
        return result;

    unsigned int commandCount = CreatorList_GetCount(group->commands);
    int index = 0;
    for (index = 0; index < commandCount; index++)
    {
        Command *commandPtr = CreatorList_GetItem(group->commands, index);
        if (commandPtr != NULL)
        {
            if (commandPtr->name != NULL)
            {
                if (strncmp(commandPtr->name, commandName, strlen(commandName)) == 0 && strlen(commandPtr->name) == strlen(commandName))
                {
                    result = commandPtr;
                }
            }
        }
    }
    return result;
}

static CommandGroup *CreatorCommand_FindCommandGroup(char* name)
{
    CommandGroup *result = NULL;
    unsigned int groupCount = CreatorList_GetCount(commandGroups);
    int index = 0;
    for (index = 0; index < groupCount; index++)
    {
        CommandGroup *groupPtr = CreatorList_GetItem(commandGroups, index);
        if (groupPtr != NULL)
        {
            if (groupPtr->name == NULL)
            {
                if (name == NULL)
                {
                    result = groupPtr;
                }
            }
            else
            {
                if (strncmp(groupPtr->name, name, strlen(groupPtr->name)) == 0)
                {
                    result = groupPtr;
                }
            }
        }
    }
    return result;
}

static bool CreatorCommand_FreeArguments(int argc, char **argv)
{
    bool result = false;
    if (argv != NULL)
    {
        unsigned int index = 0;
        for (index = 0; index < argc; index++)
        {
            if (argv[index] != NULL)
            {
                Creator_MemFree((void**) &argv[index]);
            }
        }
        Creator_MemFree((void**) &argv);
        result = true;
    }
    return result;
}

static char *CreatorCommand_GetCommandAtHistoryIndex(int index)
{
    char * result = NULL;
    if (commandHistory != NULL && index >= 0 && index < CreatorList_GetCount(commandHistory))
    {
        result = CreatorList_GetItem(commandHistory, index);
    }
    return result;
}

static int CreatorCommand_GetCommandHistoryCursorIndex(void)
{
    return commandHistoryCursorPosition;
}

bool CreatorCommand_Init(void)
{
    bool result = false;

    if (commandModuleInitialised)
        return result;

    lineTerminator = Creator_MemAlloc(sizeof(char) * strlen(DEFAULT_LINE_TERMINATOR) + 1);
    if (lineTerminator != NULL)
    {
        memcpy(lineTerminator, DEFAULT_LINE_TERMINATOR, strlen(DEFAULT_LINE_TERMINATOR) + 1);
    }
    else
    {
        return result;
    }

    CreatorCommand_ClearCommandBuffer();

    if (commandHistory == NULL)
    {
        commandHistory = CreatorList_New(COMMAND_HISTORY_LENGTH);
        if (commandHistory != NULL)
        {
            if (CreatorConsole_Init())
            {
                commandGroups = CreatorList_New(INITIAL_COMMAND_GROUP_CAPACITY);
                if (commandGroups != NULL)
                {
                    // Create default (ungrouped) command group
                    CommandGroup* ungroupedCommandGroup = CreatorCommand_CreateNewGroup_Internal(NULL, "Ungrouped commands");
                    if (ungroupedCommandGroup != NULL)
                    {
                        commandModuleInitialised = true;

                        if (CreatorCommand_RegisterCommand(NULL, "help", "help <optional group name>", CreatorCommand_DisplayHelp, true) &&
                            CreatorCommand_RegisterCommand(NULL, "history", "display recent command history", CreatorCommand_PrintCommandHistory, true))
                        {
                            CreatorCommand_DisplayPrompt();
                            result = true;
                        }
                    }
                    else
                    {
                        CreatorList_Free(&commandGroups, false);
                    }
                }
            }
        }
    }
    return result;
}

static bool CreatorCommand_IsWhiteSpace(char data)
{
    return data == ' ' || data == '\t' || data == 0;
}

static char **CreatorCommand_ParseArguments(char *command, int *argc)
{
    char **result = NULL;
    CreatorList args = CreatorList_New(5);
    char *commandEndPtr = command + strlen(command);

    char *cursor = command;
    while (cursor < commandEndPtr)
    {
        // Skip leading whitespace in command
        while (CreatorCommand_IsWhiteSpace(*command))
            command++;

        cursor = command;

        while (!CreatorCommand_IsWhiteSpace(*cursor))
            cursor++;

        int argLen = cursor - command;
        char *newArg = Creator_MemAlloc(argLen + 1);
        if (newArg != NULL)
        {
            memset(newArg, 0, argLen + 1);
            strncpy(newArg, command, argLen);
            CreatorList_Add(args, newArg);
        }
        command += argLen;
    }

    *argc = CreatorList_GetCount(args);
    if (*argc > 0)
    {
        int argTableSize = sizeof(char*) * (*argc);
        result = Creator_MemAlloc(argTableSize);
        if (result != NULL)
        {
            unsigned int index = 0;
            memset(result, 0, argTableSize);
            for (index = 0; index < *argc; index++)
            {
                result[index] = CreatorList_GetItem(args, index);
            }
        }
    }
    CreatorList_Free(&args, false);

    return result;
}

static bool CreatorCommand_PrintCommandHistory(int argc, char **argv)
{
    if (commandHistory != NULL)
    {
        CreatorCommand_PrintLineTerminators(1);
        CreatorConsole_Printf("Command History:");
        CreatorCommand_PrintLineTerminators(1);
        int historyCount = CreatorList_GetCount(commandHistory);
        int index = 0;
        for (index = 0; index < historyCount; index++)
        {
            CreatorConsole_Printf(" %c %s", PROMPT_CHARACTER, CreatorList_GetItem(commandHistory, index));
            CreatorCommand_PrintLineTerminators(1);
        }
        CreatorCommand_PrintLineTerminators(1);
    }
    return true;
}

static void CreatorCommand_PrintLineTerminators(int count)
{
    while (count > 0)
    {
        CreatorConsole_Printf(lineTerminator);
        count--;
    }
}

bool CreatorCommand_PromptUser(void)
{
    char data = 0;
    bool result = false;
    bool done = false;

    if (!commandModuleInitialised)
        return result;

    do
    {
        data = CreatorConsole_Getc();
        CreatorConsole_Printf("%c", data);
        if (data == 'y' || data == 'Y')
        {
            result = true;
            done = true;
        }

        if (data == 'n' || data == 'N')
        {
            result = false;
            done = true;
        }
        CreatorConsole_Printf(lineTerminator);
    } while (!done);
    CreatorConsole_Printf(lineTerminator);
    return result;
}

bool CreatorCommand_PromptUserWithQuery(char *query)
{
    if (!commandModuleInitialised)
        return false;

    if (query != NULL)
        CreatorConsole_Puts(query);
    return CreatorCommand_PromptUser();
}

int32_t CreatorCommand_ReadInputIntegerOption(uint32_t inputRange, bool zeroBased, bool obscureCharacters)
{
    int32_t result = -1;
    bool inputFlag = false;
    if (inputRange > 0)
    {
        uint32_t maxSetting = 0;
        if (zeroBased)
            maxSetting = inputRange - 1;
        else
            maxSetting = inputRange;

#define READ_LENGTH (10)
        uint8_t readBuffer[READ_LENGTH + 1];
        do
        {
            memset(readBuffer, '\0', READ_LENGTH + 1);
            uint32_t byteCount = 0;
            char c = '\0';

            if (!inputFlag)
                inputFlag = true;
            else
            {
                CreatorCommand_PrintLineTerminators(1);
                CreatorConsole_Puts("Invalid input, please provide a selection within the valid range: ");
                CreatorCommand_PrintLineTerminators(1);
            }

            while (byteCount < READ_LENGTH)
            {
                c = CreatorConsole_Getc();
                if (c >= '0' && c <= '9')
                {
                    CreatorConsole_Printf("%c", obscureCharacters ? 'X' : c);
                    readBuffer[byteCount] = c;
                    byteCount++;
                }
                else if (c == 127 && byteCount > 0)
                {
                    CreatorConsole_Printf("\b \b");
                    byteCount--;
                    readBuffer[byteCount] = '\0';
                }
                else if ((c == '\n') || (c == '\r'))
                {
                    CreatorConsole_Printf("\n\r");
                    break;
                }
                else
                {
                    // Invalid character
                }
            }
            if (byteCount > 0)
            {
                result = atoi((const char *) readBuffer);
                if (!zeroBased && result == 0)
                    result = -1;
            }
        } while (result > maxSetting);
    }
    return result;
}

int32_t CreatorCommand_ReadInputIntegerOptionWithQuery(char *query, uint32_t inputRange, bool zeroBased, bool obscureCharacters)
{
    CreatorConsole_Printf(query);
    return CreatorCommand_ReadInputIntegerOption(inputRange, zeroBased, obscureCharacters);
}

int32_t CreatorCommand_ReadInputString(uint8_t* buffer, uint32_t buffSize, bool obscureCharacters)
{
    int32_t byteCount = -1;
    if (buffer != NULL && buffSize > 0)
    {
        byteCount = 0;
        memset(buffer, '\0', buffSize);

        char c = '\0';
        while (byteCount < buffSize - 1)
        {
            c = CreatorConsole_Getc();
            if (c == 127 && byteCount > 0)
            {
                if (byteCount > 0)
                {
                    CreatorConsole_Printf("\b \b");
                    byteCount--;
                    buffer[byteCount] = '\0';
                }
            }
            else if (CreatorCommand_ValidCharacter(c))
            {
                if (c == '\n' || c == '\r')
                {
                    CreatorConsole_Printf("\n\r");
                    break;
                }
                else
                {
                    CreatorConsole_Printf("%c", obscureCharacters ? 'X' : c);
                    buffer[byteCount] = c;
                    byteCount++;
                }
            }
        }
    }
    return byteCount;
}

int32_t CreatorCommand_ReadInputStringWithQuery(char *query, uint8_t* buffer, uint32_t buffSize, bool obscureCharacters)
{
    CreatorConsole_Printf(query);
    return CreatorCommand_ReadInputString(buffer, buffSize, obscureCharacters);
}

bool CreatorCommand_RegisterCommand(char *groupName, char *name, char *description, CreatorCommand_Handler handler, bool allowRemoteCommand)
{
    bool result = false;
    int index = 0;

    if (!commandModuleInitialised)
        return result;

    // Validate inputs
    if (name == NULL || description == NULL || handler == NULL)
        return result;

    int nameLength = strlen(name);
    if (nameLength == 0 || nameLength > MAX_COMMAND_NAME_LENGTH)
        return result;

    int descriptionLength = strlen(description);
    if (descriptionLength == 0 || descriptionLength > MAX_COMMAND_DESCRIPTION_LENGTH)
        return result;

    unsigned int groupCount = CreatorList_GetCount(commandGroups);
    CommandGroup *targetGroupPtr = NULL;

    // Search for command name in all groups. If a pre-existing command is present then return false.
    // Also, find target group to add new command to (if one exists)
    for (index = 0; index < groupCount; index++)
    {
        CommandGroup *group = CreatorList_GetItem(commandGroups, index);
        if (CreatorCommand_FindCommand(group, name) != NULL)
        {
            return result;		// Error, pre-existing command found
        }

        if (targetGroupPtr == NULL)
        {
            if (group->name == NULL)
            {
                if (groupName == NULL)
                {
                    targetGroupPtr = group;
                }
            }
            else if (strncmp(group->name, groupName, strlen(group->name)) == 0)
            {
                targetGroupPtr = group;
            }
        }
    }

    if (targetGroupPtr != NULL && targetGroupPtr->commands != NULL)
    {
        // Create a new command object
        Command *newCommand = Creator_MemAlloc(sizeof(Command));
        if (newCommand != NULL)
        {
            newCommand->handler = handler;
            newCommand->allowRemoteCommand = allowRemoteCommand;
            newCommand->name = Creator_MemAlloc(nameLength + 1);
            if (newCommand->name != NULL)
            {
                memcpy(newCommand->name, name, nameLength + 1);
                newCommand->description = Creator_MemAlloc(descriptionLength + 1);
                if (newCommand->description != NULL)
                {
                    memcpy(newCommand->description, description, descriptionLength + 1);

                    // Add new command to the group
                    if (CreatorList_Add(targetGroupPtr->commands, (void*) newCommand))
                    {
                        result = true;
                    }
                }
            }
        }

        // Clean up failure
        if (result == false)
        {
            if (newCommand != NULL)
            {
                if (newCommand->description != NULL)
                    Creator_MemFree((void**) &newCommand->description);

                if (newCommand->name != NULL)
                    Creator_MemFree((void**) &newCommand->name);

                Creator_MemFree((void**) &newCommand);
            }
        }

    }
    else
    {
        // Command group not found
    }

    return result;
}

bool CreatorCommand_RegisterCommandGroup(char *groupName, char *groupDescription)
{
    if (!commandModuleInitialised)
        return false;

    int descriptionLength = strlen(groupDescription);
    if (descriptionLength == 0 || descriptionLength > MAX_COMMAND_GROUP_DESCRIPTION_LENGTH)
        return false;

    // Find target group to add new command to (if one exists)
    CommandGroup *targetGroupPtr = NULL;
    unsigned int groupCount = CreatorList_GetCount(commandGroups);
    int index = 0;
    for (index = 0; index < groupCount; index++)
    {
        CommandGroup *group = CreatorList_GetItem(commandGroups, index);
        if (group->name == NULL)
        {
            if (groupName == NULL)
            {
                targetGroupPtr = group;
                break;
            }
        }
        else if (strncmp(group->name, groupName, strlen(groupName)) == 0)
        {
            targetGroupPtr = group;
            break;
        }
    }

    if (targetGroupPtr == NULL)
    {
        // No group match, so create a new group...
        targetGroupPtr = CreatorCommand_CreateNewGroup_Internal(groupName, groupDescription);
    }

    return targetGroupPtr != NULL;
}

static void CreatorCommand_RollBackThroughCommandHistory(void)
{
    if (commandHistory != NULL)
    {
        if (commandHistoryCursorPosition > 0)
            commandHistoryCursorPosition--;

        if (commandHistoryCursorPosition <= 0)
            commandHistoryCursorPosition = CreatorList_GetCount(commandHistory) - 1;
    }
}

static void CreatorCommand_RollForwardThroughCommandHistory(void)
{
    if (commandHistory != NULL)
    {
        if (commandHistoryCursorPosition < CreatorList_GetCount(commandHistory))
            commandHistoryCursorPosition++;

        if (commandHistoryCursorPosition > CreatorList_GetCount(commandHistory) - 1)
            commandHistoryCursorPosition = 0;
    }
}

bool CreatorCommand_SetLineTerminator(char *newLineTerminator)
{
    bool result = false;

    if (!commandModuleInitialised)
        return result;

    if (newLineTerminator == NULL)
        return result;
    else
    {
        char *tempPtr = Creator_MemAlloc(strlen(newLineTerminator) + 1);
        if (tempPtr != NULL)
        {
            memcpy(tempPtr, newLineTerminator, strlen(newLineTerminator) + 1);
            if (lineTerminator != NULL)
                Creator_MemFree((void**) &lineTerminator);
            lineTerminator = tempPtr;
            result = true;
        }
    }
    return result;
}

bool CreatorCommand_Task(void)
{
    static char dataHistory[CHAR_HISTORY_LEN] =
    { 0 };
    static char* data = &dataHistory[CHAR_HISTORY_LEN - 1];			// Insert new characters into history here
    static char* lastData = &dataHistory[CHAR_HISTORY_LEN - 2];		// Character before the last inserted

    if (commandModuleInitialised)
    {
        if (CreatorConsole_Ready())
        {
            unsigned int index = 0;
            for (index = 0; index < CHAR_HISTORY_LEN - 1; index++)
            {
                dataHistory[index] = dataHistory[index + 1];
            }
            *data = CreatorConsole_Getc();

            if (*data == '\n' || *data == '\r')
            {
                CreatorCommand_PrintLineTerminators(1);
                if (strlen(commandBufferStart) > 0)
                {
                    CreatorCommand_AddCommandToHistory(commandBufferStart);
                    if (!CreatorCommand_ExecuteCommand(commandBufferStart, false))
                    {
                        CreatorConsole_Printf("Invalid command '%s'.", commandBufferStart);
                        CreatorCommand_PrintLineTerminators(2);
                    }
                }

                CreatorCommand_ClearCommandBuffer();
                if (!commandModuleQuit)
                    CreatorConsole_Printf("%c ", PROMPT_CHARACTER);
            }
            else if (*data == 27)
            {
                if ((dataHistory[1] == UP_KEY || dataHistory[1] == DOWN_KEY) && dataHistory[0] == 91)
                {
                    bool rollBackward = dataHistory[1] == UP_KEY;		// 'Up' key
                    bool rollForward = dataHistory[1] == DOWN_KEY;		// 'Down' key

                    if (rollBackward)
                    {
                        CreatorCommand_RollBackThroughCommandHistory();
                    }
                    else if (rollForward)
                    {
                        CreatorCommand_RollForwardThroughCommandHistory();
                    }

                    unsigned int scratchCommandLength = strlen(commandBufferStart);
                    memset(commandBufferStart, 0, CREATOR_COMMAND_MAX_LENGTH);
                    if (scratchCommandLength > 0)
                    {
                        char *whiteSpaceBuffer = Creator_MemAlloc(scratchCommandLength * 3 + 1);
                        if (whiteSpaceBuffer != NULL)
                        {
                            memset(whiteSpaceBuffer, '\b', scratchCommandLength);
                            memset(whiteSpaceBuffer + scratchCommandLength, ' ', scratchCommandLength);
                            memset(whiteSpaceBuffer + scratchCommandLength * 2, '\b', scratchCommandLength);
                            whiteSpaceBuffer[scratchCommandLength * 3] = 0;

                            CreatorConsole_Printf("%s", whiteSpaceBuffer);
                            Creator_MemFree((void**) &whiteSpaceBuffer);
                        }
                    }

                    char* historicCommand = CreatorCommand_GetCommandAtHistoryIndex(CreatorCommand_GetCommandHistoryCursorIndex());
                    int historicCommandLength = strlen(historicCommand);
                    if (historicCommand != NULL)
                    {
                        memcpy(commandBufferStart, historicCommand, historicCommandLength);
                        commandBufferCursor = commandBufferStart + historicCommandLength;
                    }
                    CreatorConsole_Printf("%s", commandBufferStart);
                }
            }
            else if (*data == BACKSPACE_KEY)
            {
                // Backspace
                if (CurrentBufferSize > 0)
                {
                    CreatorConsole_Printf("\b \b");

                    commandBufferCursor--;
                    *commandBufferCursor = 0;
                }
            }
            else if (CreatorCommand_ValidCharacter(*data) && *lastData != 91)
            {
                if (CurrentBufferSize <= CREATOR_COMMAND_MAX_LENGTH)
                {
                    *commandBufferCursor++ = *data;
                    CreatorConsole_Printf("%c", *data);
                }
                else
                {
                    CreatorCommand_PrintLineTerminators(1);
                    CreatorConsole_Printf("Maximum command length reached. Retry.");
                    CreatorCommand_PrintLineTerminators(2);
                    CreatorCommand_ClearCommandBuffer();
                    CreatorCommand_DisplayPrompt();
                }
            }
        }
    }
    else
    {
        return false;
    }

    return !commandModuleQuit;
}

static bool CreatorCommand_ValidCharacter(char data)
{
	bool result = false;
    if ((data >= 'a' && data <= 'z') || (data >= 'A' && data <= 'Z') ||
        (data >= '0' && data <= '9') ||
        (data == ' ') || (data == '-') || (data == '_') ||
        (data == ':') || (data == '/') || (data == '.') ||
        (data == '\t') || (data == '\r') || (data == '\r'))
    {
        result = true;
    }
    return result;
}
