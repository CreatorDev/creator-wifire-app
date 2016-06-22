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


#ifndef CREATOR_TASK_SCHEDULER_H_
#define CREATOR_TASK_SCHEDULER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "creator/core/base_types.h"

typedef uint CreatorTaskID;

/**
 * Task to execute.
 *
 * @param taskID identifier of executing task
 * @param context value of context passed to \ref CreatorScheduler_ScheduleTask.
 */
typedef void (*CreatorScheduler_TaskCallback)(CreatorTaskID taskID, void *context);

#define CREATOR_TASKID_INVALID (0)

/**
 * \brief Initializes the task scheduler library.
 *
 * Called by \ref CreatorCore_Initialise.
 *
 * @return true if successful, false otherwise
 */
bool CreatorScheduler_Initialise(void);

/**
 * Schedule a task for execution.
 *
 * @param executor the method that will execute the task
 * @param context context pointer that will be passed to \a executor
 * @param delayBeforeExecution delay in second before the execution must be triggered
 * @param continuous if true, the task will be executed every \a delayBeforeExecution
 * @return the identifier of the task
 */
CreatorTaskID CreatorScheduler_ScheduleTask(CreatorScheduler_TaskCallback executor, void *context, ulong delayBeforeExecution, bool continuous);

/**
 * \brief Sets a tasks recurrant execution interval.
 *
 * TODO
 *
 * @param taskID the identifier returned by \ref CreatorScheduler_ScheduleTask
 * @param delay in seconds before execution is triggerd
 */
void CreatorScheduler_SetTaskInterval(CreatorTaskID taskID, ulong delayBeforeExecution);


/**
 * \brief Removes a task from scheduling.
 *
 * If the task is already executing as this gets called, the task will finish its execution nonetheless.
 * However further recurrences of the task will be cancelled.
 *
 * @param taskID the identifier returned by \ref CreatorScheduler_ScheduleTask
 */
void CreatorScheduler_UnscheduleTask(CreatorTaskID taskID);

/**
 * \brief Frees the task scheduler library.
 *
 * Called by \ref CreatorCore_Shutdown.
 */
void CreatorScheduler_Shutdown(void);


#ifdef __cplusplus
}
#endif

#endif /* CREATOR_TASK_SCHEDULER_H_ */
