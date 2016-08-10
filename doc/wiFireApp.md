
![](../img.png)
----


# The Creator WiFire application
This section applies to the *ChipKIT WiFire development board* and describes:  

* WiFire configuration mode: behaviour, WiFi connectivity and bootstrap server settings 
* WiFire application mode: behaviour, supported resources and services  
  

## WiFire application basics
The board supports two modes of operation:  

* **Configuration mode** – the default power up mode adopted when the board is not configured with WiFi, bootstrap or application settings. Configuration mode uses SoftAP to connect directly to a mobile device for the purpose of acquiring WiFi settings. Once set up for WiFi connectivity and boostrap server connection, the board can be configured to use application mode on the next reset.  
* **Application mode** – the normal working mode of the device. The device will automatically connect to a WiFi network, connect to a bootstrap server and obtain a device server URI. Once in application mode, the only way to return to configuration mode is via the *reboot* executable resource stating SoftAP as a command parameter.  


## Configuration mode

In configuration mode the board's WiFi functionality defaults to SoftAP and will connect directly to a mobile device in order to configure WiFi and bootstrap settings. The WiFire's LEDs will indicate configuration mode status during power up:

| On | Flashing | Description |
|-----|-----|-----|
| -- | L1 | SoftAP mode. Awaiting connection to mobile application |  
| L1 | L2 | SoftAP mode. Mobile application connected in configuration mode |  
|||| 


### WiFi settings  

| WiFi network settings | type | description |  
|-----|-----|-----|  
| WiFi SSID | string | WiFi network name |  
| WiFi Encryption | string | WPA2, WEP, ... |  
| WiFi Password | string | WiFi network password |  
| Network addressing scheme | string | DHCP, Static |  
| Static: <ul><li>DNS server</li><li>IPv4 address</li><li>subnet mask</li><li>default gateway</li></ul> | strings | Static mode only |  
||||  


### Bootstrap server settings  

The WiFire device also needs to be provided with a boostrap server URI and the security credentials required to access bootstrap services. On power up in *application mode*, the device will connect to the bootstrap server at the given URI. The bootstrap server will then respond with a management server (device server) URI and suitable authentication credentials. The WiFire device will then connect to the given management server.
<br>

 
| Bootstrap server settings | type | description |  
|-----|-----|-----|  
| BootstrapURI | string | Bootstrap server URI |  
| SecurityMode | string | <ul><li>"NoSec" = no security (for internal debug testing only)</li><li>"PSK" = Pre-Shared Key  mode</li><li>"Cert" = Certficate mode</li></ul> |  
| PublicKey | string | PSK identity - PSK mode only (Security "Public Key or Identity" resource). Equivalent to client identity |  
| PrivateKey | string | Pre-Shared Key (device specific) - PSK mode only |  
| Certificate | string | Device certificate (X.509 - ASN.1 in PEM format) - Cert mode only (Security "Public Key or Identity" resource). Client can extract identity and private Keycheck value (Security "Secret Key" resource) from the certificate. |  
| BootstrapCertChain | string | Certificate chain of trust for client to trust the bootstrap server |  
||||
 
**Note**: the last 4 parameters can be mapped to the LWM2M security resource (ObjectID = 0):  

| | | |  
|-----|-----|-----|  
| SecurityMode | integer<br>(0 .. 3) | <ul><li>0 = Pre-Shared Key mode</li><li>1 = Raw Public Key mode</li><li>2 = Certificate mode</li><li>3 = NoSec mode</li></ul> |  
| PublicKey | string | Client’s Certificate (Certificate mode), raw public key cert (RPK mode) or PSK Identity (PSK mode). |  
| SecretKey | string | Secret Pre-Shared Key (PSK mode), private key (RPK or cert mode) |  
| Server PublicKey | string | Server’s Certificate (Certificate mode), server raw public key (RPK mode). |  
||||


<br>
Once the WiFi connectivity and bootstrap settings are in place the WiFire application boot mode setting can be configured:  
<br>

| Creator WiFire app settings | type | description |  
|-----|-----|-----|  
| Boot in configuration mode | boolean | <ul><li>True - boot in configuration mode</li><li>False - boot in application mode</li></ul> |  
||||

## Application mode

### Startup  
After configuration the Creator WiFire device will reboot into application mode and will then: 
 
* Initialise the WiFi  
* Bootstart using the bootstrap server (LWM2M/CoAP)  
* Register with the management/device server  
* Support commands for registered resources (LWM2M/CoAP)  

The WiFire's LEDs will indicate the level of startup progress such that:


| On | Flashing | Description |
|-----|-----|-----|
| L1 .. L2 | L3 | Establishing network connection (DHCP, awaiting IP address)|  
| L1 .. L3 | L4 | WiFire connected to network, awaiting LWM2M registration |  
| L1 .. L4 | -- | LWM2M registered |  
||||  



Supported operations and resources: 
 
* **Read:** 
    * Button states  
	* CPU temperature  
	* Potentiometer voltage  
* **Write:** LED state (for LEDs 1-4) 
* **Execute:** reboot (normal, or into SoftAP mode)  

The Creator WiFire application supports LWM2M objects to enable the device server to control resources over CoAP, (see *WiFire supported objects*).

Secure CoAP communication will be supported over DTLS (UDP transport with an encrypted payload) using the PSK or Certificate provided in configuration mode.  

----

### Next

[Supported IPSO objects](supportedObjects.md)  

### Previous

[Back to README](../README.md)  

----

----
