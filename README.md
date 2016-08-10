
![](img.png)

---

## The Creator WiFire application  

The Creator WiFire application runs on a [ChipKIT WiFire](http://www.microchip.com/Developmenttools/ProductDetails.aspx?PartNO=TDGL021-2#utm_source=MicroSolutions&utm_medium=Link&utm_term=FY17Q1&utm_content=ThirdParty&utm_campaign=Article) development board and is designed to demonstrate IoT protocols and capabilities.

## Dependencies  
The complete IoT demonstration requires:
* a device, or several devices, to monitor and manage (in this case the WiFire)  
* a device management server supporting suitable Lightweight machine to machine (LWM2M) protocols (see the [Creator device server](https://github.com/CreatorDev/DeviceServer))  
* a mobile application to configure and manage devices remotely via the device server (see the Creator mobile application)  

## WiFire application specific dependencies
This application uses the [Awa LightweightM2M](https://github.com/FlowM2M/AwaLWM2M) implementation of the OMA Lightweight M2M protocol to provide a secure and standards compliant device management solution without the need for an intimate knowledge of M2M protocols. The Awa libraries are automatically loaded in with the WiFire application build.

## Documentation  
The full documentation detailing a suitable, free of charge developer environment and application build instructions are available [here.](doc/wiFireDeveloperEnvironment.md)  
An application overview, a list of IPSO supported objects, and console setup guide are available [here.](doc/wiFireApp.md)  

---

## License  
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



----

----

