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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "creator/core/creator_memalloc.h"
#include "creator/core/common_messaging_defines.h"
#include "creator/core/common_messaging_main.h"
#include "common_messaging_parser.h"

int32 CreatorCommonMessaging_HandleContent(CreatorCommonMessaging_ControlBlock *controlBlock, char *contentBuffer, int *length)
{
    ushort *remaingContentLength = &(controlBlock->LengthOfRemainingContent);
    int contentLength = ((*length) > (*remaingContentLength)) ? (*remaingContentLength) : (*length);
    //Call callback
    if (controlBlock->ProtocolCallBack)
        controlBlock->ProtocolCallBack(CreatorCommonMessaging_CallbackEventType_Data, NULL, contentBuffer, contentLength, controlBlock->CallbackContext);
    *length -= contentLength;
    *remaingContentLength -= contentLength;
    contentBuffer += contentLength;
    if (*remaingContentLength == 0)
    {
        controlBlock->ResponsePending = false;
        controlBlock->IsContentReadingInProgress = false;
        controlBlock->IsPacketBegining = true;
        if (controlBlock->ProtocolCallBack)
            controlBlock->ProtocolCallBack(CreatorCommonMessaging_CallbackEventType_Finished, NULL, NULL, 0, controlBlock->CallbackContext);
    }
    return contentLength;
}

int CreatorCommonMessaging_ParseMessage(CreatorCommonMessaging_ControlBlock * controlBlock, char *dataBuffer, int bufferLength)
{
    int headerValueLength;
    int lineLength;
    char *lineBuffer;
    char *valueEnd;
    char *keyEnd;
    CreatorCommonMessaging_ProtocolCallBack callBack;

    callBack = controlBlock->ProtocolCallBack;
    lineBuffer = dataBuffer;
    if (controlBlock->IsPacketBegining)
    {
        // TODO - beware strchr can return NULL - ignore if \n not found...
        int searchLength = bufferLength;
        char *lineStart = dataBuffer;
        do
        {
            if (searchLength == 0)
                lineBuffer = NULL;
            else
                lineBuffer = memchr(lineStart, '\n', searchLength);
            if (!lineBuffer)
            {
                if (searchLength == 0)
                    controlBlock->PacketOffsetLength = 0;
                else
                    controlBlock->PacketOffsetLength = bufferLength;
                return 0;
            }
            lineLength = lineBuffer - lineStart - 1;
            if ((lineLength == 0) && *lineStart == '\r')
            {
                searchLength -= 2;
                lineStart += 2;
            }
            else
            {

                *(lineBuffer - 1) = '\0';
                bool validResponse = true;
                if (callBack)
                    validResponse = callBack(CreatorCommonMessaging_CallbackEventType_Response, NULL, lineStart, lineLength, controlBlock->CallbackContext);
                if (validResponse)
                {
                    ++lineBuffer;
                    bufferLength -= (lineBuffer - dataBuffer);
                    controlBlock->IsPacketBegining = false;
                    break;
                }
                else
                {
                    searchLength -= (lineLength + 1);
                    lineStart = (lineBuffer + 1);
                }
            }
        } while (true);
    }

    controlBlock->PacketOffsetLength = 0;

    while (lineBuffer && bufferLength > 0)
    {
        if (*lineBuffer == '\r' && *(lineBuffer + 1) == '\n')
        {
            // TODO - skip header event if isContentReadingInProgress == true?
            if (callBack)
                callBack(CreatorCommonMessaging_CallbackEventType_HeaderEnd, NULL, NULL, 0, controlBlock->CallbackContext);
            bufferLength -= 2;
            lineBuffer += 2;
            if (controlBlock->LengthOfRemainingContent > 0)
            {
                controlBlock->IsContentReadingInProgress = true;
                lineBuffer += CreatorCommonMessaging_HandleContent(controlBlock, lineBuffer, &bufferLength);
            }
            else
            {
                controlBlock->ResponsePending = false;
                controlBlock->IsPacketBegining = true;
                if (callBack)
                    callBack(CreatorCommonMessaging_CallbackEventType_Finished, NULL, NULL, 0, controlBlock->CallbackContext);
            }
            if (bufferLength > 0)
            {
                return (CreatorCommonMessaging_ParseMessage(controlBlock, lineBuffer, bufferLength));
            }
            continue;
        }
        if ((keyEnd = strchr(lineBuffer, ':')) == NULL)
            goto incomplete_header;
        if ((valueEnd = strchr(keyEnd + 1, '\n')) == NULL)
            goto incomplete_header;
        if (*(valueEnd - 1) != '\r')
        {
            goto incomplete_header;
        }
        //Strip off white spaces
        *keyEnd = '\0';
        keyEnd++;
        while (*keyEnd == ' ')
            keyEnd++;
        //creator_trim_white_spaces(hdr_start,keyEnd-hdr_start);
        lineLength = ((valueEnd - lineBuffer) + 1);
        //Skip CR
        valueEnd--;
        //Skip trailing spaces
        *(valueEnd) = '\0';
        headerValueLength = valueEnd - keyEnd;
        if (callBack)
            callBack(CreatorCommonMessaging_CallbackEventType_Header, lineBuffer, keyEnd, headerValueLength, controlBlock->CallbackContext);
        if (((*lineBuffer == 'C' || *lineBuffer == 'c') && strcasecmp(lineBuffer, "Content-Length") == 0))
        {
            controlBlock->LengthOfRemainingContent = (int)strtol(keyEnd, NULL, 10);
        }
        bufferLength -= lineLength;
        lineBuffer += lineLength;
    }
    return 0;
    incomplete_header:
    //Incomplete header
    controlBlock->PacketOffsetLength = bufferLength;
    char *incompleteDataBuffer = (char *)Creator_MemAlloc(controlBlock->TemporaryDataBufferLength + CREATOR_MAX_PACKET_LEN);
    if (incompleteDataBuffer)
    {
        memcpy(incompleteDataBuffer, lineBuffer, controlBlock->PacketOffsetLength);
        //Free previously allocated temporary buffer
        if (controlBlock->TemporaryDataBuffer)
            Creator_MemFree((void **)&controlBlock->TemporaryDataBuffer);
        controlBlock->TemporaryDataBufferLength += CREATOR_MAX_PACKET_LEN;
        controlBlock->TemporaryDataBuffer = incompleteDataBuffer;
    }

    return 0;

}
