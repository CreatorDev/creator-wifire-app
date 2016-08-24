
![](../img.png)
----
## IPSO objects used in the Creator WiFire application 

This guide describes the object definitions used in the Creator Wifire application.  

### Standard LWM2M objects  


| ObjectID | Name | Description |
|-----|-----|-----|
| 0 | Security | Bootstrap server could override configured security settings. |
| 1 | Server | Device server LWM2M settings - set by the Bootstrap server |
| 2 | Access Control | Access control - set by the bootstrap server. WiFire currently ignores this. |
| 3 | Device | Device object - default values set by the application. WiFire currently ignores the device reboot command.<br>Read only values provided by WiFire: <ul><li>Manufacturer</li><li>Device type</li><li>Serial number</li><li>Software version</li></ul> |  
||||  


### Standard IPSO smart objects for WiFire resources  

| Object/ID | Name | Description |  
|-----|-----|-----|  
| 3200 | IPSO Digital Input | InstanceIDs 0-1 = WiFire buttons 1..2. Device server can *observe* (subscribe to) button presses. |  
| 3311 | IPSO Light Control | InstanceIDs 0-3 = WiFire LEDs 1..4. |  
| 3202 | IPSO Analogue Input | InstanceID 0 = WiFire input voltage from potentiometer (0 to 3.3V). Server can read current value + min/max values (since startup or last reset_stats command) |  
| 3303 | IPSO Temperature Sensor | InstanceID 0 = Read temperature from WiFire's PIC32MZ internal chip temperature sensor. The server can read current value + min/max values (since startup or last reset_stats command). Temperature is returned in Celsius (units = 'Cel'). Note: The internal temperature of the WiFire is normally ≈40ºC so this sensor is not useful for measuring ambient temperature.|  
||||  
 

 
### Custom LWM2M object types  

  

| Object/ID | Name | Description |  
|-----|-----|-----|
| 20001 | ExecuteCommand | InstanceID 0 = Execute a WiFire console command |  
||||  


 
### Custom object resource definitions


#### ExecuteCommand

**ObjectID: 20001** 
 
| Resource ID | Resource | Description | Operations | Instances | Mandatory | Type |  
|-----|-----|-----|-----|-----|-----|-----| 
| 0 | Value | Execute command to be executed. WiFire supports the commands that can be executed from the console command line with the exception of interactive commands.  | Read, write | Single | Mandatory | String |  
| 1 | Count | Number of execute commands received by WiFire. Note: both valid and invalid commands are counted | Read |  Single | Mandatory | Integer |  
||||||||  


### Standard LWM2M objects *not* currently supported by the Creator WiFire application  

| ObjectID | Name |  
|-----|-----|  
| 4 | Connectivity monitoring |  
| 5 | Firmware update |  
| 6 | Location |  
| 7 | Connectivity statistics |  
|||  

----

For further information please visit:  
* [The CreatorDev forum](https://forum.creatordev.io)  
* [CreatorDev online documentation](https://docs.creatordev.io/wifire)  

---

### Next  

[WiFire communications and console setup](wiFireHardwareSetup.md) 

### Previous  

[WiFire application overview](wiFireApp.md)  

----

----




