#ifndef MSPCOMMANDS_H_INCLUDED
#define MSPCOMMANDS_H_INCLUDED


namespace MSPCommand
{
    enum command : uint8_t
    {
        /***** Generic codes *****/
        NONE,  // No command


        /***** Host to WiFi commands *****/
        // MAC addresses
        RECEIVE_BLE_MAC,  // Send BLE MAC Address
        ASK_WIFI_MAC,  // Ask WiFi MAC Address
        // Wifi Config
        WIFI_RECEIVE_SSID,  // Send WiFi SSID
        WIFI_RECEIVE_PASSWORD,  // Send WiFi Password
        WIFI_FORGET_CREDENTIALS,  // Forget saved WiFi credentials
        WIFI_RECEIVE_STATIC_IP,  // Send Static IP
        WIFI_RECEIVE_GATEWAY,  // Send Gateway (for Static IP)
        WIFI_RECEIVE_SUBNET,  // Send Subnet (for Static IP)
        WIFI_FORGET_STATIC_CONFIG,  // Forget static IP, gateway & subnet
        // Wifi commands
        WIFI_WAKE_UP,  // Notify WiFi to wake up
        WIFI_DEEP_SLEEP,  // Deep-sleep WiFi  - optional duration (uint16_t in seconds)
        WIFI_HARD_RESET,  // WiFi Hard Reset
        WIFI_CONNECT,  // Connect WiFi using saved credentials
        WIFI_DISCONNECT,  // Disconnect WiFi and turn off WiFi STA mode
        // Data publication
        PUBLISH_RAW_DATA,  // Publish raw data
        PUBLISH_FEATURE,  // Publish features
        PUBLISH_DIAGNOSTIC,  // Publish diagnostic
        // Cloud command reception and transmission
        HOST_CONFIRM_RECEPTION,  // Confirm command reception


        /***** WiFi to Host commands *****/
        // MAC addresses
        ASK_BLE_MAC,  // Ask BLE MAC Address
        RECEIVE_WIFI_MAC,  // Send WiFi MAC Address
        // Wifi Config
        WIFI_CONFIRM_NEW_CREDENTIALS,  // Config msg reception confirmation
        WIFI_CONFIRM_NEW_STATIC_CONFIG,  // Config msg reception confirmation
        // Wifi commands
        WIFI_REQUEST_ACTION,  // Ask if WiFi should wake up
        WIFI_CONFIRM_ACTION,  // Confirm reception of command
        // Wifi status alerting
        WIFI_ALERT_NO_SAVED_CREDENTIALS,  // Alert WiFi has no saved credential
        WIFI_ALERT_CONNECTED,  // Alert WiFi is connected
        WIFI_ALERT_DISCONNECTED,  // Alert WiFi is disconnected
        WIFI_ALERT_SLEEPING,  // Notify WiFi is sleeping
        // Data publication
        WIFI_CONFIRM_PUBLICATION,  // Confirm publication of data
        // Cloud command reception and transmission
        CONFIG_FORWARD_CMD,  // Forward a command string received by WiFi
        OTA_INITIATE_UPDATE,  // Initiate OTA update
        OTA_RECEIVE_DATA,  // receive a packet of OTA data
    };
}

#endif // MSPCOMMANDS_H_INCLUDED
