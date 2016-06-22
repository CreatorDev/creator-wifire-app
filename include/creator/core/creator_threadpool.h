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

/*! \file creator_threadpool.h
 */

#ifndef CREATOR_THREADPOOL_H_
#define CREATOR_THREADPOOL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "creator/core/base_types.h"

/**
 * \class CreatorThreadPool
 * Abstract threading from platform specific implementation
 */
typedef void *CreatorThreadPool;

/**
 * \memberof CreatorThreadPool
 * Task to schedule thread pool to execute
 * @param context context passed to \ref CreatorThreadPool_AddTask
 */
typedef void (*CreatorThreadPool_Callback)(void *context);


/**
 * \memberof CreatorThreadPool
 * \brief Releases the identifier and resources allocated to a thread pool.
 *
 * @param self thread pool to free
 */
void CreatorThreadPool_Free(CreatorThreadPool *self);

/**
 * \memberof CreatorThreadPool
 * Creates a new thread pool.
 *
 * @param minThreads pool has at least this many threads
 * @param maxThreads pool has at most this many threads
 * @param priority used to determine amount of CPU time each thread in pool receives.
 * @param stackSize stack size of each thread in pool.
 * @return the handler to the thread pool
 */
CreatorThreadPool CreatorThreadPool_New(uint minThreads, uint maxThreads, uint priority, uint stackSize);

/**
 * \memberof CreatorThreadPool
 * \brief Add a task/method to be executed by thread pool.
 *
 * @param self thread pool to execute task on.
 * @param runnable method/task that will be executed by thread pool.
 * @param context parameter to pass to the \a runnable.
 * @return if task was successfully added to thread pool
 */
bool CreatorThreadPool_AddTask(CreatorThreadPool self, CreatorThreadPool_Callback runnable, void *context);

#ifdef __cplusplus
}
#endif

#endif /* CREATOR_THREADPOOL_H_ */
