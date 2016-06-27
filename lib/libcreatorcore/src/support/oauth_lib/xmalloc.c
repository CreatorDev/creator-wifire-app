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

#include <string.h>

#include "xmalloc.h"
#include "creator/core/creator_memalloc.h"

void *xmalloc(size_t size)
{
    return Creator_MemAlloc(size);
}

void *xcalloc(size_t nmemb, size_t size)
{
    return Creator_MemCalloc(nmemb, size);
}

void *xrealloc(void *ptr, size_t size)
{
    return Creator_MemRealloc(ptr, size);
}

void xfree(void *buffer)
{
    Creator_MemSafeFree(buffer);
}

char *xstrdup(const char *s)
{
    void *ptr = xmalloc(strlen(s) + 1);
    strcpy(ptr, s);
    return (char*)ptr;
}
