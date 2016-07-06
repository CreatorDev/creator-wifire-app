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

/*! \file config_store.c
 *  \brief Store configuration in non-volatile memory.
 */

/*******************************************************************************
  Configuration Store module

  File Name:
    config_store.c

  Summary:
    APIs for storing and reading configuration information from non-volatile memory.

  Description:
 
 *******************************************************************************/

#include "config_store.h"
#include "device_serial.h"

#ifdef MICROCHIP_PIC32
#include "driver/nvm/drv_nvm.h"
#include "system/random/sys_random.h"
#include "system/debug/sys_debug.h"
// Coherent malloc/free support
#include <xc.h>
#else
#include "assert.h"
#include <time.h>
#include <string.h>
#endif

#include "creator/core/creator_memalloc.h"
#include "creator/core/creator_debug.h"
#include "creator/core/creator_nvs.h"
#include "creator/core/creator_threading.h"

#define CONFIG_STORE_MAGIC_NUMBER (uint64_t) 0x0101020305080D15

// WEP key generation
#define WEP_KEY_NUMBER 4
#define WEP_KEY_LENGTH_BYTES 5

typedef enum
{
    NetworkConfiguration_NotSet = 0, NetworkConfiguration_Confirmed = 1, NetworkConfiguration_Set = 0xFF
} NetworkConfigurationMode;

// Device config read from & written to the config_store
static ConfigStruct _DeviceConfig;
static LoggingSettingsStruct _LoggingSettings;
static DeviceServerConfigStruct _DeviceServerConfig;
static CreatorSemaphore _ConfigStoreLock;

static const char *_EncryptionNames[WiFiEncryptionType_Max] =
{ "WEP", "WPA", "WPA2", "Open" }; // Definition must match 'WiFiEncryptionType' enum in config_store.h

static const char *_SecurityModeNames[ServerSecurityMode_Max] =
{ "NoSec", "PSK", "Cert" }; // Definition must match 'ServerSecurityMode' enum in config_store.h

static const char *_LoggingLevelNames[CreatorLogLevel_Max] =
{ "0 - None", "1 - Error only", "2 - Error, Warning", "3 - Error, Warning, Info", "4 - All" }; // Definition must match 'loggingLevelType' enum in config_store.h

static const char *_AddressingSchemeNames[AddressScheme_Max] =
{ "dhcp", "static" }; // Definition must match 'AddressScheme' enum in config_store.h

/********************************
 * Local functions
 */
static uint8_t GetCheckbyte(const uint8_t *ptr, int length)
{
    uint32_t result = 0;
    if (ptr)
    {
        while (length > 0)
        {
            result ^= *ptr;
            ptr++;
            length--;
        }
    }
    return result;
}

static uint8_t ComputeConfigCheckbyte(void)
{
    int length = (int)((void *)&_DeviceConfig.Checkbyte - (void *)&_DeviceConfig);
    uint8_t result = GetCheckbyte((uint8_t *)&_DeviceConfig, length);
    return result;
}

static uint8_t ComputeLoggingSettingsCheckbyte(void)
{
    int length = (int)((void *)&_LoggingSettings.Checkbyte - (void *)&_LoggingSettings);
    uint8_t result = GetCheckbyte((uint8_t *)&_LoggingSettings, length);
    return result;
}

static uint8_t ComputeDeviceServerSettingsCheckbyte(void)
{
    int length = (int)((void *)&_DeviceServerConfig.Checkbyte - (void *)&_DeviceServerConfig);
    uint8_t result = GetCheckbyte((uint8_t *)&_DeviceServerConfig, length);
    return result;
}

bool GenerateWEPKey64(char *passphrase, unsigned char k64[WEP_KEY_NUMBER][WEP_KEY_LENGTH_BYTES])
{
    char pseed[4] =
    { 0 };
    uint32_t randNumber, tmp;
    uint32_t i, j;

    for (i = 0; i < strlen((char*) passphrase); i++)
    {
        pseed[i % 4] ^= (char) passphrase[i];
    }

    randNumber = pseed[0] | (pseed[1] << 8) | (pseed[2] << 16) | (pseed[3] << 24);

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 5; j++)
        {
            randNumber = (randNumber * 0x343fd + 0x269ec3) & 0xffffffff;
            tmp = (randNumber >> 16) & 0xff;
            k64[i][j] = (char) tmp;
        }
    }
    return true;
}

bool GenerateNewSoftAPPassword(char *buff, uint32_t buffSize)
{
    #define SOFTAP_PASSPHRASE_LENGTH	(16)

	const char charSet[] =
    {
        ' ','1','2','3','4','5','6','7','8','9','0',
        'a','b','c','d','e','f','g','h','i','j','k','l','m',
        'n','o','p','q','r','s','t','u','v','w','x','y','z',
        'A','B','C','D','E','F','G','H','I','J','K','L','M',
        'N','O','P','Q','R','S','T','U','V','W','X','Y','Z'
    };
    if (!buff)
        return false;

    if ((buffSize * sizeof(char)) < (SOFTAP_PASSPHRASE_LENGTH + 1) * sizeof(char))
        return false;

    memset(buff, 0, buffSize);

    char passphase[SOFTAP_PASSPHRASE_LENGTH + 1];
    int iteration;
    for (iteration = 0; iteration < SOFTAP_PASSPHRASE_LENGTH; iteration++)
    {
#ifdef MICROCHIP_PIC32
        uint32_t randVal = SYS_RANDOM_PseudoGet();
#else
        srand(time(NULL));
        uint32_t randVal = rand();
#endif
        uint8_t *_randVal = (uint8_t*) &randVal;
        int charIndex = 0;
        int byteCount;
        for (byteCount = 0; byteCount < 4; byteCount++)
        {
            charIndex += *_randVal++;
            _randVal++;
        }
        charIndex %= sizeof(charSet);
        passphase[iteration] = charSet[charIndex];
    }
    passphase[SOFTAP_PASSPHRASE_LENGTH] = '\0';

    // Generate WEP key
    unsigned char keys[WEP_KEY_NUMBER][WEP_KEY_LENGTH_BYTES];
    GenerateWEPKey64(passphase, keys);

    // Convert key-1 to a hexidecimal-string for use
    uint32_t byteIndex;
    for (byteIndex = 0; byteIndex < WEP_KEY_LENGTH_BYTES; byteIndex++)
    {
        sprintf((char*) buff, "%02X", keys[0][byteIndex]);
        buff += 2;
    }
    *buff = '\0';

    return true;
}


/********************************
 * External Functions
 */

bool ConfigStore_Config_IsValid(void)
{
	return (ConfigStore_Config_IsMagicValid() && (_DeviceConfig.Checkbyte == ComputeConfigCheckbyte()));
}

bool ConfigStore_DeInitialize(void)
{
	bool result = false;

	return result;
}

bool ConfigStore_Config_Erase(void)
{
    // TODO - could erase current config? (to limit max stack size): review usage... use malloc if needed
    ConfigStruct configStruct;
    memset(&configStruct, 0, sizeof(ConfigStruct));
    return CreatorNVS_Write(CONFIGSETTINGS_PAGEOFFSET, &configStruct, sizeof(ConfigStruct) );
}

AddressScheme ConfigStore_GetAddressingScheme(void)
{
    return _DeviceConfig.AddressingScheme;
}

const char *ConfigStore_GetAddressingSchemeName(AddressScheme addressingScheme)
{
    const char *result = NULL;
    if (addressingScheme < AddressScheme_Max)
        result = _AddressingSchemeNames[addressingScheme];
    return result;
}

const char *ConfigStore_GetDeviceName(void)
{
    return _DeviceConfig.DeviceName;
}

const char *ConfigStore_GetDeviceType(void)
{
    return _DeviceConfig.DeviceType;
}

WiFiEncryptionType ConfigStore_GetEncryptionType(void)
{
    return _DeviceConfig.Encryption;
}

const char *ConfigStore_GetMacAddress(void)
{
    return _DeviceConfig.MacAddress;
}

const char *ConfigStore_GetNetworkSSID(void)
{
    return _DeviceConfig.NetworkSSID;
}

const char *ConfigStore_GetNetworkPassword(void)
{
    return _DeviceConfig.NetworkPassword;
}

uint64_t ConfigStore_GetSerialNumber(void)
{
    return _DeviceConfig.CpuSerialNumber;
}

const char *ConfigStore_GetSoftAPSSID(void)
{
    return _DeviceConfig.SoftAPSSID;
}

const char *ConfigStore_GetSoftAPPassword(void)
{
    return _DeviceConfig.SoftAPPassword;
}

const char *ConfigStore_GetStaticDNS(void)
{
    return _DeviceConfig.StatDNS;
}

const char *ConfigStore_GetStaticGateway(void)
{
    return _DeviceConfig.StatGateway;
}

const char *ConfigStore_GetStaticNetmask(void)
{
    return _DeviceConfig.StatNetmask;
}

const char *ConfigStore_GetStaticIP(void)
{
    return _DeviceConfig.StatIP;
}

bool ConfigStore_GetStartInConfigurationMode(void)
{
    return _DeviceConfig.StartInConfigurationMode;
}

const char *ConfigStore_GetKeyValue(const char *keyName, size_t *valueLength)
{
    const char *result = NULL;
    if (valueLength)
        *valueLength = 0;
    result = (const char *) CreatorNVS_Get(keyName, valueLength);
    return result;
}

const char *ConfigStore_GetBootstrapURL(void)
{
    return _DeviceServerConfig.BootstrapURL;
}

ServerSecurityMode ConfigStore_GetSecurityMode(void)
{
    return _DeviceServerConfig.SecurityMode;
}

const char *ConfigStore_GetSecurityModeName(ServerSecurityMode securityMode)
{
    const char *result = NULL;
    if (securityMode < ServerSecurityMode_Max)
        result = _SecurityModeNames[securityMode];
    return result;
}

const char *ConfigStore_GetPublicKey(void)
{
    return _DeviceServerConfig.PublicKey;
}

const char *ConfigStore_GetPrivateKey(void)
{
    return _DeviceServerConfig.PrivateKey;
}

const char *ConfigStore_GetCertificate(void)
{
    return CreatorNVS_GetCacheAddress(DEVICESERVERSETTINGS_CERTIFICATE_OFFSET);
}

const char *ConfigStore_GetBootstrapCertChain(void)
{
    return CreatorNVS_GetCacheAddress(DEVICESERVERSETTINGS_BOOTSTRAPCHAINCERT_OFFSET);
}

bool ConfigStore_Initialize(void)
{
    bool result = false;

    CreatorNVS_Initialise();
    _ConfigStoreLock = CreatorSemaphore_New(1, 0);
    if (_ConfigStoreLock)
    {
        CreatorSemaphore_Release(_ConfigStoreLock, 1);
        result = true;
    }
    return result;
}

bool ConfigStore_Config_IsMagicValid(void)
{
    return *((uint64_t*) _DeviceConfig.Magic) == CONFIG_STORE_MAGIC_NUMBER ;
}

bool ConfigStore_Config_Read(void)
{
    return CreatorNVS_Read(CONFIGSETTINGS_PAGEOFFSET, &_DeviceConfig, sizeof(ConfigStruct));
}

bool ConfigStore_Config_ResetToDefaults(void)
{
    memset((void*) &_DeviceConfig, 0, sizeof(ConfigStruct));

    *((uint64_t*) _DeviceConfig.Magic) = CONFIG_STORE_MAGIC_NUMBER;
    _DeviceConfig.MemFormatVer = CONFIGSTORE_CONFIG_MEM_FORMAT_VERSION;

    _DeviceConfig.CpuSerialNumber = DeviceSerial_GetCpuSerialNumberUint64();
    _DeviceConfig.Encryption = WiFiEncryptionType_WPA2;

    if (!GenerateNewSoftAPPassword(_DeviceConfig.SoftAPPassword, CONFIG_STORE_DEFAULT_FIELD_LENGTH + 1))
        SYS_ERROR(SYS_ERROR_ERROR, "ConfigStore: Failed to generate a new SoftAP password!\r\n");

    _DeviceConfig.AddressingScheme = AddressScheme_Dhcp;

    memcpy(&_DeviceConfig.DeviceName, CREATOR_BLANK_DEVICE_NAME, strlen(CREATOR_BLANK_DEVICE_NAME));
    memcpy(&_DeviceConfig.DeviceType, CREATOR_DEFAULT_DEVICE_TYPE, strlen(CREATOR_DEFAULT_DEVICE_TYPE));

    _DeviceConfig.StartInConfigurationMode = 0xFF;
    _DeviceConfig.Checkbyte = ComputeConfigCheckbyte();

    return true;
}

bool ConfigStore_SetAddressingScheme(const AddressScheme addressingScheme)
{
    bool result = false;
    if (addressingScheme < AddressScheme_Max)
    {
        _DeviceConfig.AddressingScheme = addressingScheme;
        result = true;
    }
    return result;
}

bool ConfigStore_SetDeviceName(const char *value)
{
    bool result = false;
    if (value)
    {
        uint32_t valueLength = strlen((char*) value);
        memset((void*) _DeviceConfig.DeviceName, 0, CONFIG_STORE_DEFAULT_FIELD_LENGTH);
        if (valueLength > 0)
        {
            if (CONFIG_STORE_DEFAULT_FIELD_LENGTH < valueLength)
                memcpy((void*) _DeviceConfig.DeviceName, (void*) value, CONFIG_STORE_DEFAULT_FIELD_LENGTH);
            else
                memcpy((void*) _DeviceConfig.DeviceName, (void*) value, valueLength);
        }
        result = true;
    }
    return result;
}

bool ConfigStore_SetDeviceType(const char *value)
{
    bool result = false;
    if (value)
    {
        if (strcmp(_DeviceConfig.DeviceType, value) != 0)
        {
            uint32_t valueLength = strlen((char*) value);
            memset((void*) _DeviceConfig.DeviceType, 0, CONFIG_STORE_DEFAULT_FIELD_LENGTH);
            if (valueLength > 0)
            {
                if (CONFIG_STORE_DEFAULT_FIELD_LENGTH < valueLength)
                    memcpy((void*) _DeviceConfig.DeviceType, (void*) value, CONFIG_STORE_DEFAULT_FIELD_LENGTH);
                else
                    memcpy((void*) _DeviceConfig.DeviceType, (void*) value, valueLength);
            }
        }
        result = true;
    }
    return result;
}

bool ConfigStore_SetMacAddress(const char *value)
{
    bool result = false;
    if (value)
    {
        memcpy(_DeviceConfig.MacAddress, value, MAC_ADDRESS_LENGTH);
        result = true;
    }
    return result;
}

bool ConfigStore_SetNetworkConfigSet(bool isSet)
{
    if (isSet)
        _DeviceConfig.NetworkConfigConfigured = NetworkConfiguration_Set;
    else
        _DeviceConfig.NetworkConfigConfigured = NetworkConfiguration_NotSet;
    return true;
}

bool ConfigStore_SetNetworkConfigConfirmed(bool isSet)
{
    bool result = false;
    // Check config is still set (for safety only)
    if (ConfigStore_GetNetworkConfigSet())
    {
        if (isSet)
            _DeviceConfig.NetworkConfigConfigured = NetworkConfiguration_Confirmed;
        else
            _DeviceConfig.NetworkConfigConfigured = NetworkConfiguration_Set;
        result = true;
    }
    return result;
}

bool ConfigStore_SetNetworkEncryption(WiFiEncryptionType encryption)
{
    bool result = false;
    if (encryption < WiFiEncryptionType_Max)
    {
        _DeviceConfig.Encryption = encryption;
        ConfigStore_SetNetworkConfigConfirmed(false);
        result = true;
    }
    return result;
}

bool ConfigStore_SetNetworkPassword(const char *value)
{
    bool result = false;
    if (value)
    {
        uint32_t valueLength = strlen((char*) value);
        memset((void*) _DeviceConfig.NetworkPassword, 0, CONFIG_STORE_DEFAULT_FIELD_LENGTH);
        if (valueLength > 0)
        {
            if (CONFIG_STORE_DEFAULT_FIELD_LENGTH < valueLength)
                memcpy((void*) _DeviceConfig.NetworkPassword, (void*) value, CONFIG_STORE_DEFAULT_FIELD_LENGTH);
            else
                memcpy((void*) _DeviceConfig.NetworkPassword, (void*) value, valueLength);
        }
        ConfigStore_SetNetworkConfigConfirmed(false);
        result = true;
    }
    return result;
}

const char *ConfigStore_GetEncryptionName(WiFiEncryptionType encryption)
{
    const char *result = NULL;
    if (encryption < WiFiEncryptionType_Max)
        result = _EncryptionNames[encryption];
    return result;
}

bool ConfigStore_SetNetworkSSID(const char *value)
{
    bool result = false;
    if (value)
    {
        uint32_t valueLength = strlen((char*) value);
        memset((void*) _DeviceConfig.NetworkSSID, 0, CONFIG_STORE_DEFAULT_FIELD_LENGTH);
        if (valueLength > 0)
        {
            if (CONFIG_STORE_DEFAULT_FIELD_LENGTH < valueLength)
                memcpy((void*) _DeviceConfig.NetworkSSID, (void*) value, CONFIG_STORE_DEFAULT_FIELD_LENGTH);
            else
                memcpy((void*) _DeviceConfig.NetworkSSID, (void*) value, valueLength);
        }
        ConfigStore_SetNetworkConfigConfirmed(false);
        result = true;
    }
    return result;
}

bool ConfigStore_SetResetToConfigurationMode(bool value)
{
    if (value)
        _DeviceConfig.StartInConfigurationMode = 0xFF;
    else
        _DeviceConfig.StartInConfigurationMode = 0x00;

    return true;
}

bool ConfigStore_SetSoftAPPassword(const char *value)
{
    bool result = false;
    if (value)
    {
        uint32_t valueLength = strlen((char*) value);
        memset((void*) _DeviceConfig.SoftAPPassword, 0, CONFIG_STORE_DEFAULT_FIELD_LENGTH);
        if (valueLength > 0)
        {
            if (CONFIG_STORE_DEFAULT_FIELD_LENGTH < valueLength)
                memcpy((void*) _DeviceConfig.SoftAPPassword, (void*) value, CONFIG_STORE_DEFAULT_FIELD_LENGTH);
            else
                memcpy((void*) _DeviceConfig.SoftAPPassword, (void*) value, valueLength);
        }
        result = true;
    }
    return result;
}

bool ConfigStore_SetStaticDNS(const char *value)
{
    bool result = false;
    if (value)
    {
        uint32_t valueLength = strlen((char*) value);
        memset((void*) _DeviceConfig.StatDNS, 0, IPV4_ADDRESS_LENGTH);
        if (valueLength > 0)
        {
            if (IPV4_ADDRESS_LENGTH < valueLength)
                memcpy((void*) _DeviceConfig.StatDNS, (void*) value, IPV4_ADDRESS_LENGTH);
            else
                memcpy((void*) _DeviceConfig.StatDNS, (void*) value, valueLength);
        }
        result = true;
    }
    return result;
}

bool ConfigStore_SetStaticGateway(const char *value)
{
    bool result = false;
    if (value)
    {
        uint32_t valueLength = strlen((char*) value);
        memset((void*) _DeviceConfig.StatGateway, 0, IPV4_ADDRESS_LENGTH);
        if (valueLength > 0)
        {
            if (IPV4_ADDRESS_LENGTH < valueLength)
                memcpy((void*) _DeviceConfig.StatGateway, (void*) value, IPV4_ADDRESS_LENGTH);
            else
                memcpy((void*) _DeviceConfig.StatGateway, (void*) value, valueLength);
        }
        result = true;
    }
    return result;
}

bool ConfigStore_SetStaticIP(const char *value)
{
    bool result = false;
    if (value)
    {
        uint32_t valueLength = strlen((char*) value);
        memset((void*) _DeviceConfig.StatIP, 0, IPV4_ADDRESS_LENGTH);
        if (valueLength > 0)
        {
            if (IPV4_ADDRESS_LENGTH < valueLength)
                memcpy((void*) _DeviceConfig.StatIP, (void*) value, IPV4_ADDRESS_LENGTH);
            else
                memcpy((void*) _DeviceConfig.StatIP, (void*) value, valueLength);
        }
        result = true;
    }
    return result;
}

bool ConfigStore_SetStaticNetmask(const char *value)
{
    bool result = false;
    if (value)
    {
        uint32_t valueLength = strlen((char*) value);
        memset((void*) _DeviceConfig.StatNetmask, 0, IPV4_ADDRESS_LENGTH);
        if (valueLength > 0)
        {
            if (IPV4_ADDRESS_LENGTH < valueLength)
                memcpy((void*) _DeviceConfig.StatNetmask, (void*) value, IPV4_ADDRESS_LENGTH);
            else
                memcpy((void*) _DeviceConfig.StatNetmask, (void*) value, valueLength);
        }
        result = true;
    }
    return result;
}

bool ConfigStore_SoftAPSSIDValid(void)
{
    uint32_t ssidLen = strlen((const char*) _DeviceConfig.SoftAPSSID);
    return (ssidLen > 0) && (ssidLen <= CONFIG_STORE_DEFAULT_FIELD_LENGTH);
}

bool ConfigStore_StartInConfigMode(void)
{
    // TODO - duplicates GetStartInConfigurationMode
    return _DeviceConfig.StartInConfigurationMode != 0x00;
}

bool ConfigStore_SetBootstrapURL(const char *value)
{
    bool result = false;
    if (value)
    {
        uint32_t valueLength = strlen((char*) value);
        memset((void*) _DeviceServerConfig.BootstrapURL, 0, BOOTSTRAP_URL_LENGTH);
        if (valueLength > 0)
        {
            if (BOOTSTRAP_URL_LENGTH < valueLength)
                memcpy((void*) _DeviceServerConfig.BootstrapURL, (void*) value, BOOTSTRAP_URL_LENGTH);
            else
                memcpy((void*) _DeviceServerConfig.BootstrapURL, (void*) value, valueLength);
        }
        result = true;
    }
    return result;
}

bool ConfigStore_SetSecurityMode(ServerSecurityMode securityMode)
{
    bool result = false;
    if (securityMode < ServerSecurityMode_Max)
    {
        _DeviceServerConfig.SecurityMode = securityMode;
        //ConfigStore_SetNetworkConfigConfirmed(false);
        result = true;
    }
    return result;
}

bool ConfigStore_SetPublicKey(const char *value)
{
    bool result = false;
    if (value)
    {
        uint32_t valueLength = strlen((char*) value);
        memset((void*) _DeviceServerConfig.PublicKey, 0, SECURITY_PUBLIC_KEY_LENGTH);
        if (valueLength > 0)
        {
            if (SECURITY_PUBLIC_KEY_LENGTH < valueLength)
                memcpy((void*) _DeviceServerConfig.PublicKey, (void*) value, SECURITY_PUBLIC_KEY_LENGTH);
            else
                memcpy((void*) _DeviceServerConfig.PublicKey, (void*) value, valueLength);
        }
        result = true;
    }
    return result;
}

bool ConfigStore_SetPrivateKey(const char *value)
{
    bool result = false;
    if (value)
    {
        uint32_t valueLength = strlen((char*) value);
        memset((void*) _DeviceServerConfig.PrivateKey, 0, SECURITY_PRIVATE_KEY_LENGTH);
        if (valueLength > 0)
        {
            if (SECURITY_PRIVATE_KEY_LENGTH < valueLength)
                memcpy((void*) _DeviceServerConfig.PrivateKey, (void*) value, SECURITY_PRIVATE_KEY_LENGTH);
            else
                memcpy((void*) _DeviceServerConfig.PrivateKey, (void*) value, valueLength);
        }
        result = true;
    }
    return result;
}

bool ConfigStore_SetCertificate(const char *value)
{
    bool result = false;
    if (value)
    {
        int length = strlen(value);
        if (length > SECURITY_CERT_LENGTH-1)
        {
            length = SECURITY_CERT_LENGTH-1;    // ensure null terminated
        }
        _DeviceServerConfig.CertLength = length;
        _DeviceServerConfig.CertCheckByte = GetCheckbyte((uint8_t *)value, length);
        
        // Flush old value first (in case it's longer)
        CreatorNVS_SetCache(DEVICESERVERSETTINGS_CERTIFICATE_OFFSET, 0, SECURITY_CERT_LENGTH);
        result = CreatorNVS_Write(DEVICESERVERSETTINGS_CERTIFICATE_OFFSET, value, length);
    }
    return result;
}

bool ConfigStore_SetBootstrapCertChain(const char *value)
{
    bool result = false;
    if (value)
    {
        int length = strlen(value);
        if (length > SECURITY_BOOTSTRAP_CERT_CHAIN_LENGTH-1)
        {
            length = SECURITY_BOOTSTRAP_CERT_CHAIN_LENGTH-1;    // ensure null terminated
        }
        _DeviceServerConfig.BootstrapChainCertLength = length;
        _DeviceServerConfig.BootstrapChainCertCheckByte = GetCheckbyte((uint8_t *)value, length);
        
        // Flush old value first (in case it's longer)
        CreatorNVS_SetCache(DEVICESERVERSETTINGS_BOOTSTRAPCHAINCERT_OFFSET, 0, SECURITY_BOOTSTRAP_CERT_CHAIN_LENGTH);
        result = CreatorNVS_Write(DEVICESERVERSETTINGS_BOOTSTRAPCHAINCERT_OFFSET, value, length);
    }
    return result;
}

bool ConfigStore_GetNetworkConfigConfirmed(void)
{
    return _DeviceConfig.NetworkConfigConfigured == NetworkConfiguration_Confirmed;
}

bool ConfigStore_GetNetworkConfigSet(void)
{
    // Network configuration could be set or confirmed
    return _DeviceConfig.NetworkConfigConfigured != NetworkConfiguration_NotSet;
}

bool ConfigStore_Config_UpdateCheckbyte(void)
{
    _DeviceConfig.Checkbyte = ComputeConfigCheckbyte();
    return true;
}

bool ConfigStore_Config_Write(void)
{
    return CreatorNVS_Write(CONFIGSETTINGS_PAGEOFFSET, &_DeviceConfig, sizeof(ConfigStruct) );
}

bool ConfigStore_LoggingSettings_Erase(void)
{
    LoggingSettingsStruct loggingSettingsStruct;
    memset(&loggingSettingsStruct, 0, sizeof(LoggingSettingsStruct));
    return CreatorNVS_Write(LOGGINGSETTINGS_PAGEOFFSET, &loggingSettingsStruct, sizeof(LoggingSettingsStruct) );
}

CreatorLogLevel ConfigStore_GetLoggingLevel(void)
{
    return _LoggingSettings.LoggingLevel;
}

const char *ConfigStore_GetLoggingLevelName(CreatorLogLevel level)
{
    const char *result = NULL;
    if (level < CreatorLogLevel_Max)
        result = _LoggingLevelNames[level];
    return result;
}

bool ConfigStore_LoggingSettings_IsMagicValid(void)
{
    return *((uint64_t*) _LoggingSettings.Magic) == CONFIG_STORE_MAGIC_NUMBER ;
}

bool ConfigStore_LoggingSettings_IsValid(void)
{
    return ConfigStore_LoggingSettings_IsMagicValid() && (_LoggingSettings.Checkbyte == ComputeLoggingSettingsCheckbyte());
}

bool ConfigStore_LoggingSettings_Read(void)
{
    return CreatorNVS_Read(LOGGINGSETTINGS_PAGEOFFSET, &_LoggingSettings, sizeof(LoggingSettingsStruct));
}

bool ConfigStore_LoggingSettings_ResetToDefaults(void)
{
    memset((void*) &_LoggingSettings, 0, sizeof(LoggingSettingsStruct));

    *((uint64_t*) _LoggingSettings.Magic) = CONFIG_STORE_MAGIC_NUMBER;
    _LoggingSettings.MemFormatVer = CONFIGSTORE_LOGGINGSETTINGS_MEM_FORMAT_VERSION;

    _LoggingSettings.LoggingLevel = DEFAULT_LOGGING_LEVEL;

    _LoggingSettings.Checkbyte = ComputeLoggingSettingsCheckbyte();
    return true;
}

bool ConfigStore_LoggingSettings_UpdateCheckbyte(void)
{
    _LoggingSettings.Checkbyte = ComputeLoggingSettingsCheckbyte();
    return true;
}

bool ConfigStore_LoggingSettings_Write(void)
{
    return CreatorNVS_Write(LOGGINGSETTINGS_PAGEOFFSET, &_LoggingSettings, sizeof(LoggingSettingsStruct) );
}

bool ConfigStore_SetLoggingLevel(CreatorLogLevel value)
{
    bool result = false;
    if (value < CreatorLogLevel_Max)
    {
        _LoggingSettings.LoggingLevel = value;
        result = true;
        CreatorLog_SetLevel(value);
    }
    return result;
}

bool ConfigStore_DeviceServerConfig_Erase(void)
{
    DeviceServerConfigStruct deviceServerConfig;
    memset(&deviceServerConfig, 0, sizeof(DeviceServerConfigStruct));
    CreatorNVS_SetCache(DEVICESERVERSETTINGS_CERTIFICATE_OFFSET, 0, SECURITY_CERT_LENGTH);
    CreatorNVS_SetCache(DEVICESERVERSETTINGS_BOOTSTRAPCHAINCERT_OFFSET, 0, SECURITY_BOOTSTRAP_CERT_CHAIN_LENGTH);
    
    return CreatorNVS_Write(DEVICESERVERSETTINGS_PAGEOFFSET, &deviceServerConfig, sizeof(DeviceServerConfigStruct));
}

bool ConfigStore_DeviceServerConfig_Read(void)
{
    return CreatorNVS_Read(DEVICESERVERSETTINGS_PAGEOFFSET, &_DeviceServerConfig, sizeof(DeviceServerConfigStruct));
}

bool ConfigStore_DeviceServerConfig_ResetToDefaults(void)
{
    memset((void*) &_DeviceServerConfig, 0, sizeof(DeviceServerConfigStruct));
    CreatorNVS_SetCache(DEVICESERVERSETTINGS_CERTIFICATE_OFFSET, 0, SECURITY_CERT_LENGTH);
    CreatorNVS_SetCache(DEVICESERVERSETTINGS_BOOTSTRAPCHAINCERT_OFFSET, 0, SECURITY_BOOTSTRAP_CERT_CHAIN_LENGTH);

    _DeviceServerConfig.MemFormatVer = CONFIGSTORE_DEVICESERVERSETTINGS_MEM_FORMAT_VERSION;
    *((uint64_t*) _DeviceServerConfig.Magic) = CONFIG_STORE_MAGIC_NUMBER;

    _DeviceServerConfig.SecurityMode = ServerSecurityMode_NoSec;
    _DeviceServerConfig.Checkbyte = ComputeDeviceServerSettingsCheckbyte();
    return true;
}

bool ConfigStore_DeviceServerConfig_IsValid(void)
{
    bool result = ConfigStore_DeviceServerConfig_IsMagicValid() && (_DeviceServerConfig.Checkbyte == ComputeDeviceServerSettingsCheckbyte());
    if (result && _DeviceServerConfig.SecurityMode == ServerSecurityMode_Cert)
    {
        if (_DeviceServerConfig.CertLength > 0)
        {
            const char *cert = ConfigStore_GetCertificate();
            if (_DeviceServerConfig.CertLength != strlen(cert) || _DeviceServerConfig.CertCheckByte != GetCheckbyte(cert, _DeviceServerConfig.CertLength))
            {
                result = false;
            }
        }
        if (result && _DeviceServerConfig.BootstrapChainCertLength > 0)
        {
            const char *chainCert = ConfigStore_GetBootstrapCertChain();
            if (_DeviceServerConfig.BootstrapChainCertLength != strlen(chainCert) ||
                    _DeviceServerConfig.BootstrapChainCertCheckByte != GetCheckbyte(chainCert, _DeviceServerConfig.BootstrapChainCertLength))
            {
                result = false;
            }
        }
    }
    return result;
}

bool ConfigStore_DeviceServerConfig_IsMagicValid(void)
{
    return *((uint64_t*) _DeviceServerConfig.Magic) == CONFIG_STORE_MAGIC_NUMBER ;
}

bool ConfigStore_DeviceServerConfig_UpdateCheckbyte(void)
{
    _DeviceServerConfig.Checkbyte = ComputeDeviceServerSettingsCheckbyte();
    return true;
}

bool ConfigStore_DeviceServerConfig_Write(void)
{
    return CreatorNVS_Write(DEVICESERVERSETTINGS_PAGEOFFSET, &_DeviceServerConfig, sizeof(DeviceServerConfigStruct));
}
