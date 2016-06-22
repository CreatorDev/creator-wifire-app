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

#include <stddef.h>
#include <string.h>
#include "creator/core/base_types.h"
#include "creator/core/base_types_methods.h"
#include "creator/core/creator_list.h"
#include "creator/core/creator_memalloc.h"


const char *DEFAULT_DOMAIN = ".creatorworld.com";
char *DEFAULT_CA_CERT_CHAIN = NULL;


typedef struct DomainCertImpl
{
	char *Domain;
	char *Certificate;
}*DomainCert;

static CreatorList _Certificates = NULL;


DomainCert CreatorCert_FindCertificate(const char *domain)
{
	DomainCert result = NULL;
	if (_Certificates)
	{
		uint index;
		for (index = 0; index < CreatorList_GetCount(_Certificates); index++)
		{
			DomainCert domainCert = (DomainCert)CreatorList_GetItem(_Certificates, index);
			if (domainCert)
			{
				if (strcasecmp(domain, domainCert->Domain) == 0)
				{
					result = domainCert;
					break;
				}
			}
		}
	}
	return result;
}


char *CreatorCert_GetCertificate(const char *domain)
{
	char *result = NULL;
	if (domain)
	{
		DomainCert domainCert = CreatorCert_FindCertificate(domain);
		if (!domainCert)
		{
			char *parentDomain = strchr(domain,'.');
			domainCert = CreatorCert_FindCertificate(parentDomain);
		}
		if (domainCert)
		{
			result = domainCert->Certificate;
		}
	}
	return result;
}

bool CreatorCert_Initialise(void)
{
	bool result = false;
	_Certificates = CreatorList_New(5);
	if (_Certificates)
	{
		DomainCert domainCert = (DomainCert)Creator_MemAlloc(sizeof(struct DomainCertImpl));
		if (domainCert)
		{
			domainCert->Domain = (char *)DEFAULT_DOMAIN;
			domainCert->Certificate = (char *)DEFAULT_CA_CERT_CHAIN;
			CreatorList_Add(_Certificates, (void *)domainCert);
			result = true;
		}
	}
	return result;
}

bool CreatorCert_SetCertificate(const char *domain, const char *certificate)
{
	bool result = false;
	if (domain && *domain == '*')
	{
		domain = strchr(domain,'.');
	}
	if (domain)
	{
		DomainCert domainCert = CreatorCert_FindCertificate(domain);
		if (domainCert)
		{
			if (domainCert->Certificate && domainCert->Certificate != DEFAULT_CA_CERT_CHAIN)
				Creator_MemFree((void **)&domainCert->Certificate);
			domainCert->Certificate = CreatorString_Duplicate(certificate);
		}
		else
		{
			domainCert = (DomainCert)Creator_MemAlloc(sizeof(struct DomainCertImpl));
			if (domainCert)
			{
				domainCert->Domain = CreatorString_Duplicate(domain);
				domainCert->Certificate = CreatorString_Duplicate(certificate);
				CreatorList_Add(_Certificates, (void *)domainCert);
				result = true;
			}
		}
	}
	return result;
}

void CreatorCert_Shutdown(void)
{
	if (_Certificates)
	{
		uint index;
		for (index = 0; index < CreatorList_GetCount(_Certificates); index++)
		{
			DomainCert domainCert = (DomainCert)CreatorList_GetItem(_Certificates, index);
			if (domainCert)
			{
				if (domainCert->Domain && domainCert->Domain != DEFAULT_DOMAIN)
					Creator_MemFree((void **)&domainCert->Domain);
				if (domainCert->Certificate && domainCert->Certificate != DEFAULT_CA_CERT_CHAIN)
					Creator_MemFree((void **)&domainCert->Certificate);
				Creator_MemFree((void **)&domainCert);
			}
		}
		CreatorList_Free(&_Certificates, false);
	}
}

