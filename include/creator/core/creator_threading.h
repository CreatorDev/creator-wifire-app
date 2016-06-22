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
/** @file */
#ifndef CREATOR_THREADING_H_
#define CREATOR_THREADING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "creator/core/base_types.h"
#include "creator/core/errortype.h"

/**
 * \class CreatorSemaphore
 * Abstract semaphore from platform specific implementation
 */
typedef void *CreatorSemaphore;

/**
 * \memberof CreatorSemaphore
 * Creates a new semaphore.
 *
 * @param tokensTotal number of tokens this semaphore has
 * @param tokensTaken number of tokens that are already taken from the semaphore
 * @return the handler to the semaphore
 */
CreatorSemaphore CreatorSemaphore_New(uint tokensTotal, uint tokensTaken);

/**
 * \memberof CreatorSemaphore
 * Waits until \a tokens can be taken from the semaphore.
 *
 * @param self semaphore to take tokens from
 * @param tokens number of tokens to take from \a self
 */
void CreatorSemaphore_Wait(CreatorSemaphore self, uint tokens);

/**
 * \memberof CreatorSemaphore
 * Waits up to specified timeout until \a tokens can be taken from the semaphore.
 *
 * @param self semaphore to take tokens from
 * @param tokens number of tokens to take from \a self
 * @param milliseconds number of milliseconds to wait for
 * @return whether successfully obtained semaphore
 */
bool CreatorSemaphore_WaitFor(CreatorSemaphore self, uint tokens, uint milliseconds);

/**
 * \memberof CreatorSemaphore
 * Releases \a tokens into the semaphore.
 *
 * @param self semaphore to release tokens from
 * @param tokens number of tokens to release from \a self
 */
void CreatorSemaphore_Release(CreatorSemaphore self, uint tokens);

/**
 * \memberof CreatorSemaphore
 * Frees the resources allocated to a given semaphore.
 *
 * @param self semaphore to free
 */
void CreatorSemaphore_Free(CreatorSemaphore *self);



/**
 * \class CreatorThread
 * Abstract threading from platform specific implementation
 */
typedef void *CreatorThread;

/**
 * \memberof CreatorThread
 * Thread main method
 * @param thread thread callback is runnning on
 * @param context context passed to \ref CreatorThread_New
 */
typedef void (*CreatorThread_Callback)(CreatorThread thread, void *context);

/**
 * \brief Clear error state for current Thread.
 *
 */
void CreatorThread_ClearLastError(void);

/**
 * \memberof CreatorThread
 * \brief Releases the identifier and resources allocated to a thread.
 *
 * @param self thread to free
 */
void CreatorThread_Free(CreatorThread *self);


///**
// * \brief Get creator client for current thread.
// *
// */
//CreatorClient CreatorThread_GetClient();

/**
 * \brief Provides the error that the current Thread encountered, if any.
 *
 * @return the error status (\ref CreatorError_NoError if not error)
 */
CreatorErrorType CreatorThread_GetLastError(void);


/**
 * \memberof CreatorThread
 * \brief Return ID for thread (passing NULL defaults to current Thread).
 *
 * @param self Thread to get ID for (passing NULL defaults to current Thread).
 * @return the error status (\ref CreatorError_NoError if not error)
 */
uint CreatorThread_GetThreadID(CreatorThread self);

/**
 * \brief Wait for a thread to complete.
 *
 * @param self thread to wait for
 */
void CreatorThread_Join(CreatorThread self);

/**
 * \memberof CreatorThread
 * \brief Create a new thread that will start immediately.
 *
 * @param name for debug purposes to identify thread.
 * @param priority used to determine amount of CPU time this thread receives.
 * @param stackSize method that will be used as main method of the new thread.
 * @param runnable method that will be used as main method of the new thread.
 * @param context parameter to pass to the \a runnable
 * @return the instance of the thread
 */
CreatorThread CreatorThread_New(const char *name, uint priority, uint stackSize, CreatorThread_Callback runnable, void *context);


///**
// * \brief Set client to be used for future request on current thread.
// *
// */
//void CreatorThread_SetClient(CreatorClient client);


/**
 * \brief Set the error that the current Thread has encountered.
 *
 */
void CreatorThread_SetError(CreatorErrorType error);

/**
 * \memberof CreatorThread
 * \brief Makes the current thread (which must be passed in parameter) sleep.
 *
 * @param self id of the current thread
 * @param seconds number of seconds to sleep for
 * @return true if interrupted, false otherwise
 */
bool CreatorThread_Sleep(CreatorThread self, uint seconds);

/**
 * \memberof CreatorThread
 * \brief Makes the current thread (which must be passed in parameter) sleep.
 *
 * @param self id of the current thread
 * @param milliseconds number of milliseconds to sleep for
 * @return true if interrupted, false otherwise
 */
bool CreatorThread_SleepMilliseconds(CreatorThread self, uint milliseconds);

/**
 * \memberof CreatorThread
 * \brief Makes the current thread (which must be passed in parameter) sleep.
 *
 * @param self id of the current thread
 * @param ticks number of ticks to sleep for
 * @return true if interrupted, false otherwise
 */
bool CreatorThread_SleepTicks(CreatorThread self, uint ticks);

/**
 * \brief Wake up a thread that was previously sent to sleep by \ref CreatorThread_Sleep.
 *
 * @param self thread to wake up
 */
void CreatorThread_Wakeup(CreatorThread self);

#ifdef __cplusplus
}
#endif

#endif /* CREATOR_THREADING_H_ */
