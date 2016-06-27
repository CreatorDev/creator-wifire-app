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

#ifndef CREATOR_CORE_H_
#define CREATOR_CORE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "creator/core/base_types.h"
#include "creator/core/base_types_methods.h"
#include "creator/core/common_messaging_defines.h"
#include "creator/core/common_messaging_main.h"
#include "creator/core/core.h"
#include "creator/core/errortype.h"
#include "creator/core/error_methods.h"
#include "creator/core/creator_cert.h"
#include "creator/core/creator_debug.h"
#include "creator/core/creator_httpmethod.h"
#include "creator/core/creator_list.h"
#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_nvs.h"
#include "creator/core/creator_queue.h"
#include "creator/core/creator_random.h"
#include "creator/core/creator_task_scheduler.h"
#include "creator/core/creator_threading.h"
#include "creator/core/creator_threadpool.h"
#include "creator/core/creator_time.h"
#include "creator/core/creator_timer.h"
#include "creator/core/http_server.h"
#include "creator/core/http_query.h"
#include "creator/core/xmlparser.h"
#include "creator/core/xmltree.h"
    
#ifdef __cplusplus
}
#endif

#endif /* CREATOR_CORE_H_ */
