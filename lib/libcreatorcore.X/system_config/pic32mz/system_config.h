/*******************************************************************************
  Ethernet PHY Driver Testbench Configurations

  Company:
    Microchip Technology Inc.

  File Name:
    system_config.h

  Summary:
    Ethernet PHY driver testbench configuration definitions

  Description:
    These definitions statically define the driver's mode of operation.
*******************************************************************************/

//DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2012 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED AS IS WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
*******************************************************************************/
//DOM-IGNORE-END

#ifndef _SYSTEM_CONFIG_H
#define _SYSTEM_CONFIG_H

#ifndef _PLIB_UNSUPPORTED
/* This is a temporary workaround for an issue with the peripheral library "Exists"
   functions that causes superfluous warnings.  It "nulls" out the definition of
   The PLIB function attribute that causes the warning.  Once that issue has been
   resolved, this definition should be removed. */

     #define _PLIB_UNSUPPORTED
#endif


/*****************************************************************************
 * The system debug peripheral
 * Specify the debug port baud rate.
 *****************************************************************************/
#define SYS_DEBUG_BAUDRATE      115200

/*****************************************************************************
 * The system console enable
 * Use to enable the system console
 *****************************************************************************/
#define SYS_CONSOLE_ENABLE


/*****************************************************************************
 * The system console comm channel
 * Specify the console port.
 *****************************************************************************/
//#define SYS_CONSOLE_PORT          SYS_DEVICE_DBAPPIO
#define SYS_CONSOLE_PORT          SYS_MODULE_UART_4

/*****************************************************************************
 * The system console comm channel
 * Specify the console bit rate.
 *****************************************************************************/
#define SYS_CONSOLE_BAUDRATE     115200

/*****************************************************************************
 * The system console buffer space
 * Specify the length of the buffering for the console operations.
 *****************************************************************************/
#define SYS_CONSOLE_BUFFER_LEN   1024


// *****************************************************************************
// *****************************************************************************
// Section: NVM Driver Configuration
// *****************************************************************************
// *****************************************************************************

/* NVM Driver setup for interrupt mode */
#define DRV_NVM_INTERRUPT_MODE true

/* NVM Driver will support one instance */
#define DRV_NVM_INSTANCES_NUMBER 1

/* NVM Driver will have only one client */
#define DRV_NVM_CLIENTS_NUMBER 2

/* NVM Driver can support upto five data
 * requests */
#define DRV_NVM_BUFFER_OBJECT_NUMBER 5


#endif  // __SYSTEM_PROFILE_H_

