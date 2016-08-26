
![](../img.png)
----

# The Creator WiFire application 

## Configuration mode

This section applies to the *ChipKIT WiFire development board* and describes the Creator WiFire application's configuration mode, behaviour, WiFi connectivity and device server setup. 

### Requirements

You'll need a suitable standard USB to Mini-USB cable and a console interface (such as PuTTY) to display the WiFire's console output during setup and to use its command line interface (CLI). For hardware setup instructions see [WiFire hardware setup](wiFireHardwareSetup.md).  

### Connecting to the WiFire in SoftAP 

On initial power up, the WiFire adopts configuration mode which supports softAP (software enabled access point) enabling a mobile device to connect to it directly for the purpose of configuration. A suitable open source mobile application is available [here](https://github.com/CreatorDev/creator-wifire-mobile-app).  

When successfully configured the WiFire device will power up or reboot into application mode by default thereafter.

The device can be forced back into configuration mode by:  
* Holding down Button 1 and Button 2, while pressing the reset button  
* Entering *reboot_softap* at the command line interface  
* Issuing a reboot in softap mode command via the LWM2M interface  

On startup in configuration mode the console  will display content *similar* to the following:
```
==========================
*** WiFi Configuration ***
==========================
MAC:            001EC01E13C1
SSID:           WiFire_1E13C1
Password:       DECE79F249
Network Type:   SoftAP
Security:       WEP40
```

If you're using the [open source mobile application](https://github.com/CreatorDev/creator-wifire-mobile-app) to configure the WiFire device you'll need to:

* Login to the mobile application using developer credentials available by creating a [developer account](https://id.creatordev.io/).  
* Follow the application menu to *on-board* your device.  
* Use the CreatorDev.io device server to generate access credentials for the new device.  
* Follow the remaining setup process to enter the network SSID, password and the security mode for the WiFire.  
* Use the simple web service hosted by the Wifire device (as described below) to further configure it.  

### SoftAP configuration web service

In configuration mode the WiFire supports a simple web servicefor mobile applications to configure the WiFire application.  
**Note.** *The WiFire's web services are hosted at IP address: 192.168.1.25 by default. Currently the WiFire's configuration mode does not support DNS.*  

The WiFire's configuration settings may be read and written via HTTP request using the following endpoints...  

#### Get device info  
Returns information about the WiFire device to which the mobile application is currently connected.

Request: **GET** https://192.168.1.25/device
 
```xml
Response: 200
Content-Type: application/xml; application/vnd.imgtec.com.device-info+xml; charset=utf-8

<DeviceInfo>
  <DeviceName> -- Device Name -- </DeviceName>
  <ClientID> -- client identifier -- </ClientID>
  <DeviceType>wifire</DeviceType>
  <SerialNumber> -- Device Serial number -- </SerialNumber>
  <MACAddress> -- Device MAC address -- </MACAddress>
  <SoftwareVersion> -- Application firmware Version -- </SoftwareVersion>
</DeviceInfo>
```
**Notes:**  
* **Device Name** - a default or user assigned name for this device (default = 'wifire'). Device name is not supported by the device server. In application mode the device name can be retrieved from the custom ClientInfo object (over LWM2M).
* **Device Type** - device type is set by the application (default = ‘wifire’).
* **Client ID** - set by the security settings in the Device Server configuration (typically a Guid). The default value is empty until the device server has been configured.
* **MAC Address** - unique factory-programmed Wi-Fi module MAC address
* **Serial Number** - unique factory-programmed CPU serial number (16x hex characters)

A response will have one of the following status codes:  

| HTTP Status Code | Description |  
|-----|-----|  
| 200 | OK - the request was fulfilled |  
| | |  
<br>  

#### Set Wi-Fi network settings
Sets the WiFi settings used by the WiFire application to access the network.

Request: **POST** https://192.168.1.25/network  

```xml
Content-Type: application/xml; application/vnd.imgtec.com.network-config+xml; charset=utf-8  

<NetworkConfig>
  <SSID> -- WiFi SSID -- </SSID>
  <Encryption>WEP | WPA | WPA2 | Open</Encryption>
  <Password> -- WiFi password -- </Password>
  <AddrMethod>static | dhcp</AddrMethod>
  <StaticDNS> -- IP address -- </StaticDNS>
  <StaticIP> -- IP address -- </StaticIP>
  <StaticNetmask> -- netmask -- </StaticNetmask>
  <StaticGateway> -- IP address -- </StaticGateway>
</NetworkConfig>
```
**Notes:**  
* Bespoke mobile applications will need to prompt the user for these settings.  
* The default encryption type is WPA2

A response will have one of the following status codes:  

| HTTP Status Code | Description |  
|-----|-----|  
| 204 | No content - the request was fulfilled and no content was returned |  
| 400 | Bad request - invalid request body content or format |  
| | |  
<br>  

#### Get Wi-Fi network settings
Retrieve the current Wi-Fi network settings from the WiFire application.  

Request: **GET** https://192.168.1.25/network
```xml 
Response: 200
Content-Type: application/xml; application/vnd.imgtec.com.network-config+xml; charset=utf-8

<NetworkConfig>
  <SSID> -- WiFi SSID -- </SSID>
  <Encryption>WEP | WPA | WPA2 | Open</Encryption>
  <Password></Password>
  <AddrMethod>static | dhcp</AddrMethod>
  <StaticDNS> -- IP address -- </StaticDNS>
  <StaticIP> -- IP address -- </StaticIP>
  <StaticNetmask> -- netmask -- </StaticNetmask>
  <StaticGateway> -- IP address -- </StaticGateway>
</NetworkConfig>
```
**Notes:**  
* **Password** - this field is always empty  
* **Static address mode** - The *static* fields are only used in static mode

A response will have one of the following status codes:  

| HTTP Status Code | Description |  
|-----|-----|  
| 200 | OK - the request was fulfilled |  
| | |  
<br>  

#### Set device server
Set the device server settings for use by the WiFire application.

Request: **POST** https://192.168.1.25/deviceserver
```xml 
Response: 200
Content-Type: application/xml; application/vnd.imgtec.com.device-server+xml; charset=utf-8
<DeviceServer>
  <BootstrapUrl> -- Bootstrap server URL -- </BootstrapUrl>
  <SecurityMode>NoSec | PSK | Cert</SecurityMode>
  <PublicKey> -- PSK (Pre-Shared Key) identity -- </PublicKey>
  <PrivateKey> -- Pre-Shared Key -- </PrivateKey>
  <Certificate> -- Device specific certificate (X.509 - ASN.1 in PEM format) -- </Certificate>
  <BootstrapCertChain> -- Chain of trust for bootstrap server (X.509 - ASN.1 in PEM format) -- </BootstrapCertChain>
</DeviceServer>
```
**Notes:**  
* Bespoke mobile apps will need to get the above settings from the device server for each device before connecting to the WiFire.  
* The WiFire should normally be configured into Cert mode (the WiFire will only support PSK or NoCert modes for testing).   Other device types with less memory might need to support PSK mode instead.  
* **Bootstrap Url** - Used to bootstrap to the device server (LWM2M).  
* **Security Mode** - NoSec = No security (for testing only, not supported by device server), PSK = Pre-shared key mode, Cert = Certificate mode.  
* **Public Key** - PSK identity (PSK mode only). Provides the client identity for use with the device server
* **Private Key** - Pre-shared key (PSK mode only).  
* **Certificate** - Device specific certificate (Cert mode only). The WiFire will extract the client identity from this certificate.  
* **BootstrapCertChain** - Certificate chain of trust for client to trust the bootstrap server (optional).  
 
A response will have one of the following status codes:  

| HTTP Status Code | Description |  
|-----|-----|  
| 200 | OK - the request was fulfilled |  
| | |  
<br>  

#### Get device server  
A mobile application can retrieve information about the device server to which it is connected.

Request: **GET** https://192.168.1.25/deviceserver

```xml 
Response: 200
Content-Type: application/xml; application/vnd.imgtec.com.device-server+xml; charset=utf-8
<DeviceServer>
  <BootstrapUri> -- Bootstrap server URI -- </BootstrapUri>
  <SecurityMode>NoSec | PSK | Cert</SecurityMode>
  <PublicKey> -- PSK (Pre-Shared Key) identity -- </PublicKey>
  <PrivateKey></PrivateKey>
  <Certificate></Certificate>
  <BootstrapCertChain></BootstrapCertChain>
</DeviceServer>
```

Notes:  
* For security, **Private Key**, **Certificate** and **BootstrapCertChain** are omitted or empty when retrieved.

A response will have one of the following status codes:  

| HTTP Status Code | Description |  
|-----|-----|  
| 200 | OK - the request was fulfilled |  
| | |  
<br>  

#### Set device name  
Set that name of a device. 

Request: **POST** https://192.168.1.25/name  
```xml
Content-Type: application/xml; application/vnd.imgtec.com.device-name+xml; charset=utf-8  

<DeviceName>
  <Name> -- Device Name -- </Name>
</DeviceName>
```

Notes:  
* **Device Name** - Device name is not supported by the device server. It can only be accessed in SoftAP mode or from the USB console.  

A response will have one of the following status codes:  

| HTTP Status Code | Description |  
|-----|-----|  
| 204 | No content - the request was fulfilled and no content was returned |  
| 400 | Bad request - invalid request body content or format |  
| | |  
<br>  

#### Get device name  
Retrieve the name of a device.

Request: **GET** https://192.168.1.25/name  

```xml
Response: 200
Content-Type: application/xml; application/vnd.imgtec.com.device-name+xml; charset=utf-8  

<DeviceName>
  <Name> -- Device Name -- </Name>
</DeviceName>
```


Notes:  
* **Name** - Default value is *WiFire_xxxxxx* where *xxxxxx* is the last three bytes of the device MAC address, for example, *WiFire_1E13C1*.

A response will have one of the following status codes:  

| HTTP Status Code | Description |  
|-----|-----|  
| 200 | OK - the request was fulfilled |  
| | |  
<br> 

#### Reboot device  
Reboots the device into application mode.

Request: **POST** https://192.168.1.25/reboot  

Notes:  
* The device is rebooted after returning its response. If successful the HTTPS connection will then be closed.  
* The device will restart in application mode if both the Network and Device server settings have been configured, otherwise it will restart in configuration mode.

A response will have one of the following status codes:  

| HTTP Status Code | Description |  
|-----|-----|  
| 204 | No content - the request was fulfilled and no content was returned |  
| | |  
<br>  

#### Reboot into softAP mode
Reboots the WiFire device into softAP mode.

Request: **POST** https:// 192.168.1.25 /rebootsoftap  

Notes:  
* The device is rebooted after returning its response and will boot in configuration (softAP) mode. If successful the HTTPS connection will then be closed.  

A response will have one of the following status codes:  

| HTTP Status Code | Description |  
|-----|-----|  
| 204 | No content - the request was fulfilled and no content was returned |  
| | |  
<br>  

## Factory reset

Request: **POST** https:// 192.168.1.25 /factoryreset  

Notes:  
* All settings are cleared. The device can now be re-configured.  

A response will have one of the following status codes:  

| HTTP Status Code | Description |  
|-----|-----|  
| 204 | No content - the request was fulfilled and no content was returned |  
| | |  
<br>  

### HTTP response status codes
The web service supports the following standard HTTP status codes...  

| HTTP Status Code | Description |  
|-----|-----|  
| 200 | OK - The request was fulfilled |  
| 204 | No content - The request was fulfilled and no content was returned |  
| 400 | Bad request - The request had a bad syntax or could not be fulfilled (e.g. invalid output format specified) |  
| 401 | Unauthorized - The client should retry the request with a suitable authorization header. The parameter within the response gives a list of acceptable authorization schemes |  
| 403 | Forbidden - The specified request type at this endpoint is forbidden |  
| 404 | Not found - The request involved an endpoint that could not be found |  
| 500 | Internal error - The WiFire web service encountered an unexpected condition that prevented it from completing the request |  
| 501 | Not implemented - The WiFire web service does not support the request |  
| | |  

---

### Next

[Supported WiFire objects](supportedObjects.md)  

### Previous

[WiFire application](wiFireApp.md)  

[Back to README](../README.md)  



---

---
