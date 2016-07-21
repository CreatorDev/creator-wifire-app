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

#include <string.h>
#include "app.h"
#include "command_handlers.h"

#include "creator_command.h"
#include "config_store.h"
#include "device_serial.h"
#include "string_builder.h"
#include "app_client.h"
#include "ui_control.h"
#include "creator/creator_console.h"


//typedef enum
//{
//    app_setshow_app_values1,
//    app_setshow_app_values2,
//    app_setshow_cmd__max
//} app_SetShowCommand;
//
//typedef struct
//{
//    const char* Name;
//    bool IsSetCommand;
//    bool IsShowCommand;
//} setShowCommandInfo;

//static setShowCommandInfo app_SetShowCommands[app_setshow_cmd__max] =
//{      //   Name            Set?    Show?
//        { "show_app_values1", false, true },
//        { "show_app_values2", false, true } };

//static bool CommandBoardDetails(int argc, char** argv);

#define STATERAPP_COMMAND_GROUP "app_commands"
#define LINE_TERM "\r\n"          // line terminator

void AppCommands_Initialise(void)
{
    // TODO - Add application specific commands
    CreatorCommand_RegisterCommandGroup(STATERAPP_COMMAND_GROUP, ": Application Commands");
    //CreatorCommand_RegisterCommand(STATERAPP_COMMAND_GROUP, "board_details", "Display board information for output to label printing software", CommandBoardDetails, false);
}

bool AppCommands_Echo(int argc, char** argv)
{
    bool result = true;
    if (argc > 1)
    {
        int index;
        CreatorConsole_Puts("Echo:");
        for (index = 1; index < argc; index++)
        {
            CreatorConsole_Puts(" ");
            CreatorConsole_Puts(argv[index]);
        }
        CreatorConsole_Puts(LINE_TERM);
    }
    return result;
}

bool AppCommands_Leds(int argc, char** argv)
{
    bool result = false;
    bool allLeds = false;
    bool ledOn = false;
    int ledID = 0;
    if (argc == 3)
    {
        if (strcmp(argv[1], "all") == 0)
        {
            allLeds = true;
            result = true;
        }
        else
        {
            
            ledID = atoi(argv[1]);
            if (ledID >= 1 && ledID <= 4)
            {
                ledID--;    // convert led number to index
                result = true;
            }
        }
        
        if (result)
        {
            if (strcmp(argv[2], "on") == 0)
                ledOn = true;
            else if (strcmp(argv[2], "off") != 0)
                result = false;
        }
    }
    if (result)
    {
        if (allLeds)
        {
            for (ledID = 0; ledID < NUMBER_OF_LEDS; ledID++)
            {
                UIControl_LEDCommand(ledID, ledOn);
            }
        }
        else
        {
            UIControl_LEDCommand(ledID, ledOn);
        }
    }
    else
    {
        CreatorConsole_Puts("Invalid LED command. Syntax: 'leds <id> <on|off>', where id = all,1,2,3,4" LINE_TERM);
    }
    return result;
}

bool AppCommands_ResetStatistics(int argc, char** argv)
{
    bool result = true;
    Client_ResetStatistics();
    CreatorConsole_Printf("Statistics reset" LINE_TERM);
    return result;
}

bool AppCommands_CommandShow(int argc, char** argv)
{
    bool result = true;
//    if (argc == 2)
//    {
//        if (argv[1])
//        {
//            if (strcmp(argv[1], app_SetShowCommands[app_setshow_app_values1].Name) == 0)
//            {
//                CreatorConsole_Puts("App command1 - not supported" LINE_TERM);
//            }
//            else if (strcmp(argv[1], app_SetShowCommands[app_setshow_app_values2].Name) == 0)
//            {
//                CreatorConsole_Puts("App command2 - not supported" LINE_TERM);
//            }
//            else
//            {
//                result = false;
//            }
//        }
//        else
//        {
//            result = false;
//        }
//    }
//    else
//    {
//        if (argc > 2)
//            result = false;
//        CreatorConsole_Puts("Starter App Supported:" LINE_TERM);
//        int index;
//        for (index = 0; index < app_setshow_cmd__max; index++)
//        {
//            if (app_SetShowCommands[index].IsShowCommand)
//            {
//                CreatorConsole_Puts("\t\t");
//                CreatorConsole_Puts(app_SetShowCommands[index].Name);
//                CreatorConsole_Puts(LINE_TERM);
//            }
//        }
//    }
    return result;
}

