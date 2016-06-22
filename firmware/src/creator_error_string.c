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

#include "creator_error_string.h"
#ifndef MICROCHIP_PIC32
char* loginErrors[] = {
        "", // 0
        "There seems to be a problem with the application. Try again. If the problem persists, please restart the application.", // 1
        "There seems to be a problem with the application. Try again. If the problem persists, please restart the application.", //2
        "Access to Creator has been denied. Please check your API keys in the settings menu. The API keys can be found under your account information on the CreatorCloud web site.", //3
        "no password entered", //4
        "The username is not registered. Please check the username or sign-up for an account", //5
        "Unable to connect to Creator. Please check your network connection to ensure you have access to the internet.", //6
        "The username or password that you entered is incorrect ", //7
        "Account already in use",  //8
        "Resource Removed" // 9
        "There was a problem connecting to the server. Try again.", //10
        "There was a problem connecting to the server. Try again.", //11
        "There was a problem connecting to the server. Try again.", //12
        "Client Not logged In", //13
        "Local Storage Error", //14
        "Client Not compatible"  //15
};

char* signUpErrors[] = {
        "", // 0
        "There seems to be a problem with the application. Try again. If the problem persists, please restart the application.", // 1
        "There seems to be a problem with the application. Try again. If the problem persists, please restart the application.", //2
        "Access to Creator has been denied. Please check your API keys in the settings menu.", //3
        "no password and/or email entered", //4
        "Invalid email entered", //5
        "Unable to connect to Creator. Please check your network connection to ensure you have access to the internet.", //6
        "The email or password that you entered is incorrect ", //7
        "Username already in use. Enter another username",  //8
        "Resource Removed" // 9
        "There was a problem connecting to the server. Try again.", //10
        "There was a problem connecting to the server. Try again.", //11
        "There was a problem connecting to the server. Try again.", //12
        "Client Not logged In", //13
        "Local Storage Error",  //14
        "Client Not compatible"  //15
};

char* genericErrors[] = {
	"Object Modified", //16
        "Bad Request: Invalid Fields", //17
        "Bad Request: Invalid Pin", //18
        "Bad Request: Insufficient Funds", //19
        "Bad Request: Product Not Found", //20
        "Bad Request: Product Not Available", //21
        "Bad Request: Card Not Supported", //22
        "Bad Request: Invalid Card NUmber", //23
        "Bad Request: Invalid Card expiry date", //24
        "Bad Request: Subscription Invalid for user", //25
        "Bad Request: Subscription Already Trialled", //26
        "Bad Request: Subscripiton cannot be Trialled", //27
        "Bad Request: Subscripiton cannot be Renewed", //28
        "Bad Request: Duplicate Email", //29
        "Bad Request: Account not found", //30
        "Bad Request: Voucher not valid", //31
        "Bad Request: Voucher Already being processed", //32
        "Bad Request: Content Folder limit reached", //33
        "Unknown Request", //34
        "Client is not time-synchronized with the server", //35
        "Client session token is not valid"  //36
};
#endif
