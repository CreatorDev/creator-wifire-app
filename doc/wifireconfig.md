
![](../img.png)
----

# The Creator WiFire application
This section applies to the *ChipKIT WiFire development board* and describes the WiFire configuration mode, behaviour, WiFi connectivity and bootstrap server settings. 



### WiFi settings  

| WiFi network settings | type | description |  
|-----|-----|-----|  
| WiFi SSID | string | WiFi network name |  
| WiFi Encryption | string | WEP, WPA, WPA2 (default), Open |  
| WiFi Password | string | WiFi network password |  
| Network addressing scheme | string | DHCP, Static |  
| Static: <ul><li>DNS server</li><li>IPv4 address</li><li>subnet mask</li><li>default gateway</li></ul> | strings | Static mode only |  
||||  


### Bootstrap server settings  

The WiFire device also needs to be provided with a bootstrap server URI and the security credentials required to access bootstrap services. On power up in *application mode*, the device will connect to the bootstrap server at the given URI. The bootstrap server will then respond with a management server (device server) URI and suitable authentication credentials. The WiFire device will then connect to the given management server.
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
