/*****************************************************************************
        Copyright 2014 by Imagination Technologies.
                  All rights reserved.
                  No part of this software, either material or conceptual
                  may be copied or distributed, transmitted, transcribed,
                  stored in a retrieval system or translated into any
                  human or computer language in any form by any means,
                  electronic, mechanical, manual or otherwise, or
                  disclosed to third parties without the express written
                  permission of:
                        Imagination Technologies, Home Park Estate,
                        Kings Langley, Hertfordshire, WD4 8LZ, U.K.

*****************************************************************************/

#ifndef _CREATOR_TASK_PRIORITY_H
#define _CREATOR_TASK_PRIORITY_H

#ifdef MICROCHIP_PIC32
#include "FreeRTOS.h"
#include "task.h"
#else
#define tskIDLE_PRIORITY 0
#endif

// Task priorities
#define USER_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )	// low priority
//#define CMP_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )	// common_messaging_defines.h
#define SYSTEM_TASK_PRIORITY	( tskIDLE_PRIORITY + 4 )	// highest priority

#endif	// _CREATOR_TASK_PRIORITY_H

