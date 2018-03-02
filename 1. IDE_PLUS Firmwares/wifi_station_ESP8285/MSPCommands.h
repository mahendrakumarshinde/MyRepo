#ifndef MSPCOMMANDS_H_INCLUDED
#define MSPCOMMANDS_H_INCLUDED


namespace MSPCommand
{
    enum command : uint8_t
    {
        /***** Generic codes *****/
        NONE,  // 0: No command


        /***** Host to WiFi commands *****/
        // MAC addresses
        RECEIVE_BLE_MAC,  // 1: Send BLE MAC Address
        ASK_WIFI_MAC,  // 2: Ask WiFi MAC Address
        // Wifi Config
        WIFI_RECEIVE_SSID,  // 3: Send WiFi SSID
        WIFI_RECEIVE_PASSWORD,  // 4: Send WiFi Password
        WIFI_FORGET_CREDENTIALS,  // 5: Forget saved WiFi credentials
        WIFI_RECEIVE_STATIC_IP,  // 6: Send Static IP
        WIFI_RECEIVE_GATEWAY,  // 7: Send Gateway (for Static IP)
        WIFI_RECEIVE_SUBNET,  // 8: Send Subnet (for Static IP)
        WIFI_FORGET_STATIC_CONFIG,  // 9: Forget static IP, gateway & subnet
        // Wifi commands
        WIFI_WAKE_UP,  // 10: Notify WiFi to wake up
        WIFI_DEEP_SLEEP,  // 11: Deep-sleep WiFi  - optional duration (uint16_t in seconds)
        WIFI_HARD_RESET,  // 12: WiFi Hard Reset
        WIFI_CONNECT,  // 13: Connect WiFi using saved credentials
        WIFI_DISCONNECT,  // 14: Disconnect WiFi and turn off WiFi STA mode
        // Data publication
        PUBLISH_RAW_DATA,  // 15: Publish raw data
        PUBLISH_FEATURE,  // 16: Publish features
        PUBLISH_DIAGNOSTIC,  // 17: Publish diagnostic
        // Cloud command reception and transmission
        HOST_CONFIRM_RECEPTION,  // 18: Confirm command reception


        /***** WiFi to Host commands *****/
        // MAC addresses
        ASK_BLE_MAC,  // 19: Ask BLE MAC Address
        RECEIVE_WIFI_MAC,  // 20: Send WiFi MAC Address
        // Wifi Config
        WIFI_CONFIRM_NEW_CREDENTIALS,  // 21: Config msg reception confirmation
        WIFI_CONFIRM_NEW_STATIC_CONFIG,  // 22: Config msg reception confirmation
        // Wifi commands
        WIFI_REQUEST_ACTION,  // 23: Ask if WiFi should wake up
        WIFI_CONFIRM_ACTION,  // 24: Confirm reception of command
        // Wifi status alerting
        WIFI_ALERT_NO_SAVED_CREDENTIALS,  // 25: Alert WiFi has no saved credential
        WIFI_ALERT_CONNECTED,  // 26: Alert WiFi is connected
        WIFI_ALERT_DISCONNECTED,  // 27: Alert WiFi is disconnected
        WIFI_ALERT_SLEEPING,  // 28: Notify WiFi is sleeping
        // Data publication
        WIFI_CONFIRM_PUBLICATION,  // 29: Confirm publication of data
        // Cloud command reception and transmission
        CONFIG_FORWARD_CMD,  // 30: Forward a command string received by WiFi
        OTA_INITIATE_UPDATE,  // 31: Initiate OTA update
        OTA_RECEIVE_DATA,  // 32: Receive a packet of OTA data
    };
}

#endif // MSPCOMMANDS_H_INCLUDED
