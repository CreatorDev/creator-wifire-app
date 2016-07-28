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

// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************

#include <time.h>
#include "system/reset/sys_reset.h"

#include "app.h"
#include "command_handlers.h" // CLI command handlers
#include "server_cert.h"

#include "creator/creatorcore.h"
#include "creator/core/creator_threading.h"
#include "creator/creator_console.h"

#include "app_config.h"
#include "config_store.h"   // configuration-store  APIs
#include "device_serial.h"
#include "ui_control.h"
#include "app_commands.h"   // LED control
#include "app_client.h"
#include "adc_driver.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Application Data

  Summary:
	Holds application data

  Description:
	This structure holds the application's data.

  Remarks:
	This structure should be initialized by the APP_Initialize function.
    
	Application strings and buffers are be defined outside this structure.
 */

APP_DATA appData;

#ifndef APP_NAME
#ifdef PIC32MZ
#undef PIC32MZ
#endif
#define APP_NAME Creator PIC32MZ WiFire App
#endif

#ifndef APP_VERSION
#define APP_VERSION 0.0.20
#endif

#ifndef APP_VERSIONDATE
#define APP_VERSIONDATE 28 July 2016
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define	STARTER_APP_DEVICE_TYPE "WiFire"

static AppInfo info =
{
    .ApplicationName = TOSTRING(APP_NAME),
    .ApplicationVersion = TOSTRING(APP_VERSION),
    .ApplicationVersionDate =  TOSTRING(APP_VERSIONDATE),
    .CommandShow = AppCommands_CommandShow,
    .AppCLI_ResetHandler = CommandHandlers_ResetHandler
};

//App information.
//static time_t _PrevAppTaskTime = 0;
//#define APP_TASK_SECONDS (20)

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************

void ApplicationModeTasks(void);
bool App_Shutdown(void);


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
	void APP_Initialize ( void )

  Remarks:
	See prototype in app.h.
 */
void APP_Initialize(void)
{
    // Initialize application state machine and other parameters
    appData.state = APP_STATE_INIT;

    // Configure SPI4 to Communicate with MRF24W Wifi Module
    // 1. SDI4 = RG7
    SDI4Rbits.SDI4R = 0x02; //SDI4 = RPF5
    PLIB_PORTS_PinModePerPortSelect(PORTS_ID_0, PORT_CHANNEL_F, PORTS_BIT_POS_5, PORTS_PIN_MODE_DIGITAL);  // F5 not analog
    PLIB_PORTS_PinDirectionInputSet(PORTS_ID_0, PORT_CHANNEL_F, PORTS_BIT_POS_5);                         // F5  input
    //PORTGbits.RG7 = 1; // pull-up
    // 2. SDO4 = RPG0
    PLIB_PORTS_PinModePerPortSelect(PORTS_ID_0, PORT_CHANNEL_G, PORTS_BIT_POS_0, PORTS_PIN_MODE_DIGITAL); // G0 not analog
    RPG0Rbits.RPG0R = 0x08; //RPG0 = SDO4

    // UART 4
    TRISFCLR = (1 << 8);
    RPF8R = 0x02;
    TRISFSET = (1 << 2);
    U4RXR = 0x0B;

    SYS_INT_Enable();

    AdcDriver_Initialise();
    uint32_t randomSeed = AdcDriver_GetPotentiometerLevel();
    SYS_RANDOM_PseudoSeedSet(randomSeed);

    AppConfig_Initialise(&info);
}

/******************************************************************************
  Function:
	void APP_Tasks ( void )

  Remarks:
	See prototype in app.h.
 */
void APP_Tasks ( void )
{
    switch (appData.state) {
        // Initial state
        case APP_STATE_INIT:
        {
            bool appInitialized = true;

            AppConfig_NetworkInitialise();
            if (AppConfig_IsRunningInConfigurationMode())
            {
                AppConfig_StartConfigWebServer((uint8 *) serverCert, sizeof(serverCert));
                // TODO - check: stays in web server until next restart?
            }
            else
            {
                UIControl_SetUIState(AppUIState_AppInitConnectingToServer);
                AppConfig_CreatorInitialise();
                //UIControl_SetUIState(AppUIState_InteractiveConnectedToCreator);   // TODO - update on client connected
                AppCommands_Initialise();

                Client_Initialise();
            }
            if (appInitialized)
            {
                appData.state = APP_STATE_SERVICE_TASKS;
            }
            break;
        }

        case APP_STATE_SERVICE_TASKS:
        {
            if (!AppConfig_IsRunningInConfigurationMode())
            {
                ApplicationModeTasks();
                if (CommandHandlers_ResetToConfigurationMode())
                {
                    CreatorConsole_Puts("\r\nApplication entering configuration mode. Please wait\r\n");
                    break;
                }
            }
            AppConfig_Tasks();
            break;
        }

            // The default state should never be executed
        default:
        {
            // TODO: Handle error in application state machine
            break;
        }
    }
}

void ApplicationModeTasks(void)
{
    // Debounce switch presses for rebooting to configuration mode
    if (AppConfig_CheckForConfigurationModeRebootButtonPress())
    {
        CommandHandlers_ResetHandler(true);
    }

    // Hook to perform period tasks
    //time_t currentTime;
    //Creator_GetTime(&currentTime);
    //if ((currentTime - _PrevAppTaskTime) >= APP_TASK_SECONDS)
    //{
    //    _PrevAppTaskTime = currentTime;
    //}

    //Handle device reset request.
    if (CommandHandlers_IsResetPending())
    {
        if (App_Shutdown())
        {
            AppConfig_SoftwareReset(CommandHandlers_ResetToConfigurationMode());
        }
        else
            CreatorConsole_Puts("Shutdown failed\r\n");
    }
    else
    {
        UIControl_pollInputSensors();
    }
}

/*
 * Application shut down and initiate device reset.
 */
bool App_Shutdown(void)
{
    bool result = true;
    Client_Shutdown();

    return result;
}
