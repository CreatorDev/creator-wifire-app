/***********************************************************************************************************************
 Copyright (c) 2016, Imagination Technologies Limited
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

/*******************************************************************************
  Dynamic String Header

  File Name:
    dynstring.h

  Summary:
    APIs for manipulating dynamically growing C-strings.

  Description:

 *******************************************************************************/

#ifndef __DYNSTRING_H_
#define __DYNSTRING_H_

/*
 * A dynamically growing string type.
 *
 */

typedef void* StringBuilder;


// Add characters to a dynamic string
StringBuilder StringBuilder_Append(StringBuilder dynString, const char* str);

// Add an signed integer to a dynamic string
StringBuilder StringBuilder_AppendInt(StringBuilder dynString, int value);

// Clear a dynamic string
StringBuilder StringBuilder_Clear(StringBuilder dynString);

// Dispose of a dynamic string
void StringBuilder_Free(StringBuilder *dynString);

// Return a C-string representing the dynamic string content
const char* StringBuilder_GetCString(StringBuilder dynString);

//
// Return the length of the dynamic string content
//
// Returns: string length, or -1 if invalid
int StringBuilder_GetLength(StringBuilder dynString);

// Create a new dynamic string with the specified initial size
// Set 'initialSize' to 0 to use default
StringBuilder StringBuilder_New(unsigned int initialSize);


#endif // __DYNSTRING_H_
