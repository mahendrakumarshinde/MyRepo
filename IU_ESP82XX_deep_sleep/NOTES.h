/*
 This version of the ESP82XX WiFi/UART bridge incorporates new features:
 
 1) It sets up the IU board as a WiFi STATION, not as a WiFi ACCESS POINT
 2) There are five pre-configured IP address/UDP port combinations in "config.h"; SENSOR_0 - SENSOR_4. Uncomment one sensor
    definition onl. Multiple boards should have different sensor designations. Five were pre-defined because the host in this
    case is an ESP8285 development board configured as an access point; this board is reputed to handle at most five clients while
    configured in access point mode. In principle, the number of clients is limited by the access point
 3) Whatever kind of access point is used and whether the client IP addresses are static or dynamic, data filtering is done by UDP port.
    Each board should have a unique UDP port and the host should know to scan this port for new data...
 4) In the serial protocol, command "11" was added to support deep sleep. The "Go to sleep" message from the host includes command "11"
    designation and a single precision float indicating the number of seconds deep sleep should last
 5) The "WiFiSerial::WiFi_Bridge_Ready()" function was added to inform the host that the WiFi/UART bridge is connected to the host and ready
    to receive data. This function sends command "21" to the host to indicate "ready" status
*/
