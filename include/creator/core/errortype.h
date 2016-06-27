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

#ifndef CREATOR_ERRORTYPE_H_
#define CREATOR_ERRORTYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \class Errors
 * \brief Type of error that can be encountered during a request.
 * \see Creator_GetLastError
 */
typedef enum
{
    //! no error
    CreatorError_NoError,                                   //0
    //! an internal error occured, this is a bug that should be reported to libcreator developers
    CreatorError_Internal,                                  //1
    //! there is not enough memory to fulfill this operation
    CreatorError_Memory,                                    //2
    //! the requested method is not supported on this object
    CreatorError_MethodUnavailable,                         //3
    //! one of the method argument was invalid
    CreatorError_InvalidArgument,                           //4
    //! the resource that was requested in not available
    CreatorError_ResourceNotFound,                          //5
    //! failure to connect to the server
    CreatorError_Network,                                   //6
    //! the CreatorCient doesn't have the credentials to access this resource
    CreatorError_Unauthorised,                              //7
    //! the submitted data is in conflict with existing data on the webservice. look up the HTML documentation for more details
    CreatorError_Conflict,                                  //8
    //! the resource you're trying to access existed but has been removed
    CreatorError_Removed,                                   //9
    //! the server suffered an internal error
    CreatorError_Server,                                    //10
    //! the webservice cannot fulfill the request at the moment
    CreatorError_ServerBusy,                                //11
    //! the timed out while waiting for this request
    CreatorError_Timeout,                                   //12
    //! the CreatorClient is not logged in, there is no current user or device
    CreatorError_Anonymous,                                 //13
    //! failure to store data in the local filesystem
    CreatorError_LocalStorage,                              //14
    //! the server and client are not compatible, check that the client is up to date
    CreatorError_VersionConflict,                           //15
    //! the object you're trying to use has been modified concurrently, get a new copy
    CreatorError_ConcurrentModification,                    //16

    /* BadRequest errors */
    CreatorError_BadRequest_Min,                            //17
    CreatorError_BadRequestInvalidFields                    //17
        = CreatorError_BadRequest_Min,
    CreatorError_BadRequestInvalidPin,                      //18
    CreatorError_BadRequestInsufficientFunds,               //19
    CreatorError_BadRequestProductNotFound,                 //20
    CreatorError_BadRequestProductNotAvailable,             //21
    CreatorError_BadRequestCardNotSupported,                //22
    CreatorError_BadRequestInvalidCardnumber,               //23
    CreatorError_BadRequestInvalidCardExpiryDate,           //24
    CreatorError_BadRequestSubscriptioninvalidForUser,      //25
    CreatorError_BadRequestSubscriptionAlreadyTrialled,     //26
    CreatorError_BadRequestSubscriptionCannotBeTrialled,    //27
    CreatorError_BadRequestSubscriptionCannotBeRenewed,     //28
    CreatorError_BadRequestDuplicateEmail,                  //29
    CreatorError_BadRequestAccountNotFound,                 //30
    CreatorError_BadRequestVoucherNotValid,                 //31
    CreatorError_BadRequestVoucherAlreadyBeingProcessed,    //32
    CreatorError_BadRequestContentFolderLimitReached,       //33
    CreatorError_BadRequest_Unknown,                        //34

    //! the client is not time-synchronized with the server, \ref CreatorClient_SynchronizeServerTime must be called explicitely
    CreatorError_ServerTimeSync,                            //35
    //! the client session token is not valid, \ref CreatorClient_RenewSession must be called explicitely
    CreatorError_ExpiredSession,                            //36
} CreatorErrorType;


#ifdef __cplusplus
}
#endif

#endif /* CREATOR_ERRORTYPE_H_ */
