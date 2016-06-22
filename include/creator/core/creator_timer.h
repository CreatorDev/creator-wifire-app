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

/*! \file creator_timer.h
 *  \brief LibCreatorCore .
 */


#ifndef CREATOR_TIMER_H_
#define CREATOR_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "creator/core/base_types.h"

/**
 * \class CreatorTimer
 * Abstract timer from platform specific implementation
 */
typedef void *CreatorTimer;

/**
 * \memberof CreatorTimer
 * Timer callback function
 * @param context context passed to \ref CreatorTimer_New
 */
typedef void (*CreatorTimer_Callback)(CreatorTimer self, void *context);


/**
 * \memberof CreatorTimer
 * \brief Releases the identifier and resources allocated to a timer.
 *
 * @param self timer to free
 */
void CreatorTimer_Free(CreatorTimer *self);

/**
 * \memberof CreatorTimer
 * \brief Get the number of ticks that have pass since library initialise
 *
 */
uint CreatorTimer_GetTickCount(void);

/**
 * \memberof CreatorTimer
 * \brief Get the number of tick per second for platform
 *
 */
uint CreatorTimer_GetTicksPerSecond(void);

/**
 * \brief Initializes the timer library.
 *
 * Called by \ref CreatorCore_Initialise.
 *
 */
void CreatorTimer_Initialise(void);

/**
 * \memberof CreatorTimer
 * \brief Create a new timer.
 *
 * @param name for debug purposes to identify thread.
 * @param periodInMilliseconds time period (milliseconds) after which \a runnable is called.
 * @param continuous whether it is a one shot or continuous calls \a runnable after period has elapsed.
 * @param runnable method that will be called after period has elapsed.
 * @param context parameter to pass to the \a runnable
 * @return the instance of the timer
 */
CreatorTimer CreatorTimer_New(const char *name, uint periodInMilliseconds, bool continuous, CreatorTimer_Callback runnable, void *context);

/**
 * \memberof CreatorTimer
 * \brief Reset/restart period on timer.
 *
 * @param self timer to reset
 */
bool CreatorTimer_Reset(CreatorTimer self);



/**
 * \memberof CreatorTimer
 * \brief Change period on timer.
 *
 * @param self timer to change
 * @param periodInMilliseconds time period (milliseconds) after which timer will fire.
 */

bool CreatorTimer_SetPeriod(CreatorTimer self, uint periodInMilliseconds);

/**
 * \memberof CreatorTimer
 * \brief Set the number of tick per second for platform
 *
 * @param ticks Ticks per scond
 */
void CreatorTimer_SetTicksPerSecond(uint ticks);

/**
 * \memberof CreatorTimer
 * \brief Start timer.
 *
 * @param self timer to start
 */
bool CreatorTimer_Start(CreatorTimer self);

/**
 * \memberof CreatorTimer
 * \brief Stop timer.
 *
 * @param self timer to stop
 */
bool CreatorTimer_Stop(CreatorTimer self);


/**
 * \brief Frees the timer library.
 *
 * Called by \ref CreatorCore_Shutdown.
 */
void CreatorTimer_Shutdown(void);


#ifdef __cplusplus
}
#endif

#endif /* CREATOR_TIMER_H_ */
