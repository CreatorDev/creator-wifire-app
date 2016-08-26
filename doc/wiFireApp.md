
![](../img.png)
----


# The Creator WiFire application

## WiFire application basics
The WiFire application supports two modes of operation:  

* **Configuration mode** – the default power up mode adopted when the board is not configured with WiFi, bootstrap or application settings. Configuration mode uses SoftAP to connect directly to a mobile device for the purpose of acquiring WiFi settings. Once set up for WiFi connectivity and bootstrap server connection, the board can be configured to use application mode on the next reset.  
* **Application mode** – the normal working mode of the device. The device will automatically connect to a WiFi network, connect to a bootstrap server and obtain a device server URI. Once in application mode, the device can be made to return to configuration mode by executing the *reboot* executable resource stating SoftAP as a command parameter, or by resetting the device (using the reset button) while simultaneously pressing buttons 1 and 2.  

## Configuration mode  

In configuration mode the board's WiFi functionality defaults to SoftAP and will connect directly to a mobile device in order to configure WiFi and bootstrap settings. The WiFire's LEDs will indicate configuration mode status during power up:

| On | Flashing | Description |
|-----|-----|-----|
| -- | L1 | SoftAP mode. Awaiting connection to mobile application |  
| L1 | L2 | SoftAP mode. Mobile application connected in configuration mode |  
|||| 

For detailed config information see [WiFire configuration](wifireconfig.md)

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
* **Execute:** WiFire console commands (for example reset_stats, reboot_softap)  

The Creator WiFire application supports LWM2M objects to enable the device server to control resources over CoAP, (see [WiFire Supported objects](supportedObjects.md)).

Secure CoAP communication will be supported over DTLS (UDP transport with an encrypted payload) using the PSK or Certificate provided in configuration mode.  

---   
For further information please visit:  
* [The CreatorDev forum](https://forum.creatordev.io)  
* [CreatorDev online documentation](https://docs.creatordev.io/wifire)  

---


### Next

[Supported WiFire objects](supportedObjects.md)  

[WiFire configuration mode](wifireconfig.md)  


### Previous

[Back to README](../README.md)  

----

----
