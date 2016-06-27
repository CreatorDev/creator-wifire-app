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
  Configuration Store Header

  File Name:
    config_store.h

  Summary:
    APIs for storing and reading configuration information from non-volatile memory.

  Description:

 *******************************************************************************/

#ifndef __CONFIG_STORE_H_
#define __CONFIG_STORE_H_

#include <stdbool.h>
#include <stdint.h>

#include "creator_settings.h"
#include "activitylog.h"
#include "creator/core/base_types.h"

#define	MAC_ADDRESS_LENGTH					(12)
#define	CONFIG_STORE_DEFAULT_FIELD_LENGTH	(32)

// WiFi limits (for wifi drivers > v1.03)
#define	MIN_KEY_PHRASE_LENGTH	(8) //DRV_WIFI_ConvPassphraseToKey() - min length required
#define	MAX_KEY_PHRASE_LENGTH	(63) // DRV_WIFI_ConvPassphraseToKey() - max length required

// Configuration Memory Base Address
#define CONFIG_STORE_BASE_ADDRESS							0xBD1F0000

// Versioning
#define	CONFIGSTORE_CONFIG_MEM_FORMAT_VERSION				(1)
#define	CONFIGSTORE_LOGGINGSETTINGS_MEM_FORMAT_VERSION		(1)
#define	CONFIGSTORE_DEVICESERVERSETTINGS_MEM_FORMAT_VERSION	(1)

#define	IPV4_ADDRESS_LENGTH									(15)
#define CREATOR_ROOT_URL_LENGTH								(48)

// Device server
#define BOOTSTRAP_URL_LENGTH                                (64)    // TODO - check max lengths
#define SECURITY_PUBLIC_KEY_LENGTH							(64)
#define SECURITY_PRIVATE_KEY_LENGTH							(64)    // TODO - 256
#define SECURITY_CERT_LENGTH    							(64)    // TODO - 16K? (limit to 16K flash page - less config size)
#define SECURITY_BOOTSTRAP_CERT_CHAIN_LENGTH				(64)    // TODO - 4K?

//
// Device Config settings
//
typedef enum
{
    WiFiEncryptionType_WEP = 0,
    WiFiEncryptionType_WPA,
    WiFiEncryptionType_WPA2,
    WiFiEncryptionType_Open,
    WiFiEncryptionType_Max
} WiFiEncryptionType;

typedef enum
{
    AddressScheme_Dhcp = 0,
    AddressScheme_StaticIP,
    AddressScheme_Max
} AddressScheme;

typedef enum
{
    ServerSecurityMode_NoSec = 0,
    ServerSecurityMode_PSK = 1,
    ServerSecurityMode_Cert = 2,
    ServerSecurityMode_Max
} ServerSecurityMode;

#define DEFAULT_LOGGING_ENABLED_SETTING		(true)
#define DEFAULT_LOGGING_LEVEL				(CreatorActivityLogLevel_Information)
#define DEFAULT_LOGGING_CATEGORIES			(1<<CreatorActivityLogCategory_HardwareBoot | 1<<CreatorActivityLogCategory_Startup | 1<<CreatorActivityLogCategory_Shutdown)


/*
 * Non-volatile storage; data structure definitions
 * - ConfigStruct
 * - LoggingSettingsStruct
 */

#define NVM_STRUCTURE_SPACING_BYTES			(32)			// Todo, could improve this to make it more intelligent (i.e. word/row-aligned)?

// Format of configuration memory
//
// Notes:
// - startInConfigurationMode: == 0xFFFF -> boot into configuration mode
//			       != 0xFFFF -> boot into application mode
// - checkbyte will be XOR of magic number through to padding field
//
#define	CONFIGSETTINGS_PAGEOFFSET			(0)
typedef struct
{
    // Memory Format Information
    uint8_t             Magic[8];
    uint32_t            MemFormatVer;

    // Device Information
    uint64_t			CpuSerialNumber;
    char				DeviceName[CONFIG_STORE_DEFAULT_FIELD_LENGTH+1];		// Null terminated
    char				DeviceType[CONFIG_STORE_DEFAULT_FIELD_LENGTH+1];		//  "       "
    char				MacAddress[MAC_ADDRESS_LENGTH+1];						//  "       "

    // SoftAP Information
    char				SoftAPSSID[CONFIG_STORE_DEFAULT_FIELD_LENGTH+1];		//  "       "
    char				SoftAPPassword[CONFIG_STORE_DEFAULT_FIELD_LENGTH+1];	//  "       "

    // Network Configuration
    WiFiEncryptionType	Encryption;
    char				NetworkSSID[CONFIG_STORE_DEFAULT_FIELD_LENGTH+1];		//  "       "
    char				NetworkPassword[CONFIG_STORE_DEFAULT_FIELD_LENGTH+1];	//  "       "
    AddressScheme		AddressingScheme;

    // StaticIP configuration
    char				StatDNS[IPV4_ADDRESS_LENGTH+1];							//  "       "
    char				StatIP[IPV4_ADDRESS_LENGTH+1];							//  "       "
    char				StatNetmask[IPV4_ADDRESS_LENGTH+1];						//  "       "
    char				StatGateway[IPV4_ADDRESS_LENGTH+1];						//  "       "

    // Device Control
    uint8_t				StartInConfigurationMode;
    uint8_t				NetworkConfigConfigured;    // TODO - review/add device server configured?
    
    // Housekeeping
    uint16_t			Padding;
    uint8_t				Checkbyte;												// Note: Must always be last field in structure

} ConfigStruct;


#define	LOGGINGSETTINGS_PAGEOFFSET      (CONFIGSETTINGS_PAGEOFFSET + sizeof(ConfigStruct) + NVM_STRUCTURE_SPACING_BYTES)
typedef struct
{
    // Memory Format Information
    uint8_t					Magic[8];
    uint32_t				MemFormatVer;

    // Logging Configuration
    uint32_t				LoggingEnabled;		// Note: could reduce size to uint8 (if other settings are changed in the future)
    CreatorActivityLogLevel	LoggingLevel;
    uint16_t				LoggingCategories;

    // Housekeeping
    uint16_t				Padding;
    uint8_t					Checkbyte;												// Note: Must always be last field in structure

} LoggingSettingsStruct;

#define	DEVICESERVERSETTINGS_PAGEOFFSET     (LOGGINGSETTINGS_PAGEOFFSET + sizeof(LoggingSettingsStruct) + NVM_STRUCTURE_SPACING_BYTES)
typedef struct
{
    // Memory Format Information
    uint8_t					Magic[8];
    uint32_t				MemFormatVer;

    // 	Device server configuration
    ServerSecurityMode  SecurityMode;
    char				BootstrapURL[BOOTSTRAP_URL_LENGTH+1];                   //  Null terminated
    char				PublicKey[SECURITY_PUBLIC_KEY_LENGTH+1];                //  "       "
    char				PrivateKey[SECURITY_PRIVATE_KEY_LENGTH+1];              //  "       "
    char				Certificate[SECURITY_CERT_LENGTH+1];                    //  "       "
    char				BootstrapCertChain[SECURITY_BOOTSTRAP_CERT_CHAIN_LENGTH+1]; //  "       "

    // Housekeeping
    uint16_t				Padding;
    uint8_t					Checkbyte;												// Note: Must always be last field in structure

} DeviceServerConfigStruct;

// WARNING: must be after the last config setting
#define	CONFIG_STORE_SETTING_END        (DEVICESERVERSETTINGS_PAGEOFFSET + sizeof(DeviceServerConfigStruct))
// TODO - error if > NV flash page size


///////////////////
// External APIs //
///////////////////
bool ConfigStore_Initialize(void);
bool ConfigStore_DeInitialize(void);

//
// [Device Config]
//
bool ConfigStore_Config_Erase(void);
bool ConfigStore_Config_Read(void);
bool ConfigStore_Config_ResetToDefaults(void);
bool ConfigStore_Config_IsValid(void);
bool ConfigStore_Config_IsMagicValid(void);
bool ConfigStore_Config_UpdateCheckbyte(void);
bool ConfigStore_Config_Write(void);

// APIs
bool ConfigStore_SoftAPSSIDValid(void);
bool ConfigStore_StartInConfigMode(void);
bool ConfigStore_GetNetworkConfigConfirmed(void);       // TODO - review use in creatorstarter
bool ConfigStore_GetNetworkConfigSet(void);

AddressScheme ConfigStore_GetAddressingScheme(void);
const char* ConfigStore_GetAddressingSchemeName(AddressScheme addressingScheme);
const char* ConfigStore_GetDeviceName(void);
const char* ConfigStore_GetDeviceType(void);
const char* ConfigStore_GetEncryptionName(WiFiEncryptionType encryption);
WiFiEncryptionType  ConfigStore_GetEncryptionType(void);
const char* ConfigStore_GetMacAddress(void);
const char* ConfigStore_GetNetworkSSID(void);

// TODO - delete?
const char* ConfigStore_GetCreatorKeyValue(const char *keyName, size_t *valueLength);

const char* ConfigStore_GetNetworkPassword(void);
uint64_t ConfigStore_GetSerialNumber(void);
const char* ConfigStore_GetSoftAPSSID(void);
const char* ConfigStore_GetSoftAPPassword(void);
const char* ConfigStore_GetStaticDNS(void);
const char* ConfigStore_GetStaticGateway(void);
const char* ConfigStore_GetStaticNetmask(void);
const char* ConfigStore_GetStaticIP(void);

bool ConfigStore_GetStartInConfigurationMode(void);

bool ConfigStore_SetAddressingScheme(const AddressScheme addressingScheme);
bool ConfigStore_SetDeviceName(const char* value);
bool ConfigStore_SetDeviceType(const char* value);
bool ConfigStore_SetMacAddress(const char* value);
bool ConfigStore_SetNetworkConfigConfirmed(bool isSet);
bool ConfigStore_SetNetworkConfigSet(bool isSet);
bool ConfigStore_SetNetworkEncryption(WiFiEncryptionType encryption);
bool ConfigStore_SetNetworkPassword(const char* value);
bool ConfigStore_SetNetworkSSID(const char* value);
bool ConfigStore_SetRegistrationKey(const char* value);
bool ConfigStore_SetResetToConfigurationMode(bool value);
bool ConfigStore_SetSoftAPPassword(const char* value);
bool ConfigStore_SetStaticDNS(const char* value);
bool ConfigStore_SetStaticGateway(const char* value);
bool ConfigStore_SetStaticIP(const char* value);
bool ConfigStore_SetStaticNetmask(const char* value);


//
// [Logging Settings]
//
bool ConfigStore_LoggingSettings_Erase(void);
bool ConfigStore_LoggingSettings_Read(void);
bool ConfigStore_LoggingSettings_ResetToDefaults(void);
bool ConfigStore_LoggingSettings_IsValid(void);
bool ConfigStore_LoggingSettings_IsMagicValid(void);
bool ConfigStore_LoggingSettings_UpdateCheckbyte(void);
bool ConfigStore_LoggingSettings_Write(void);

// APIs
uint16_t ConfigStore_GetLoggingCategories(void);
const char* ConfigStore_GetLoggingCategoryName(CreatorActivityLogCategory category);
bool ConfigStore_GetLoggingEnabled(void);
CreatorActivityLogLevel ConfigStore_GetLoggingLevel(void);
const char* ConfigStore_GetLoggingLevelName(CreatorActivityLogLevel level);

bool ConfigStore_SetLoggingEnabled(bool value);
bool ConfigStore_SetLoggingLevel(CreatorActivityLogLevel value);
bool ConfigStore_SetLoggingCategories(uint16_t value);


//
// [Device Server Settings]
//
bool ConfigStore_DeviceServerConfig_Erase(void);
bool ConfigStore_DeviceServerConfig_Read(void);
bool ConfigStore_DeviceServerConfig_ResetToDefaults(void);
bool ConfigStore_DeviceServerConfig_IsValid(void);
bool ConfigStore_DeviceServerConfig_IsMagicValid(void);
bool ConfigStore_DeviceServerConfig_UpdateCheckbyte(void);
bool ConfigStore_DeviceServerConfig_Write(void);

// APIs
const char* ConfigStore_GetBootstrapURL(void);
ServerSecurityMode ConfigStore_GetSecurityMode(void);
const char* ConfigStore_GetSecurityModeName(ServerSecurityMode securityMode);
const char* ConfigStore_GetPublicKey(void);
const char* ConfigStore_GetPrivateKey(void);
const char* ConfigStore_GetCertificate(void);
const char* ConfigStore_GetBootstrapCertChain(void);

bool ConfigStore_SetBootstrapURL(const char* value);
bool ConfigStore_SetSecurityMode(ServerSecurityMode securityMode);
bool ConfigStore_SetPublicKey(const char* value);
bool ConfigStore_SetPrivateKey(const char* value);
bool ConfigStore_SetCertificate(const char* value);
bool ConfigStore_SetBootstrapCertChain(const char* value);


//
//Network-Manager
//
#ifndef MICROCHIP_PIC32
void CreatorWriteNetworkConfigFile();
#endif
#endif // __CONFIG_STORE_H_
