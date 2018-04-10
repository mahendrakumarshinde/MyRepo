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
        HOST_FORWARD_CMD,  // 19: Forward a command string received by the host
        // Publication protocol
        PUBLICATION_USE_MQTT,  // 20
        PUBLICATION_USE_HTTP, // 21
        // Settable parameters (addresses, credentials, etc)
        SET_RAW_DATA_ENDPOINT_HOST,  // 22
        SET_RAW_DATA_ENDPOINT_ROUTE,  // 23
        SET_RAW_DATA_ENDPOINT_PORT,  // 24
        SET_MQTT_SERVER_IP,  // 25
        SET_MQTT_SERVER_PORT,  // 26
        SET_MQTT_USERNAME,  // 27
        SET_MQTT_PASSWORD,  // 28

        PLACE_HOLDER_29,  // 29
        PLACE_HOLDER_30,  // 30
        PLACE_HOLDER_31,  // 31
        PLACE_HOLDER_32,  // 32
        PLACE_HOLDER_33,  // 33
        PLACE_HOLDER_34,  // 34
        PLACE_HOLDER_35,  // 35
        PLACE_HOLDER_36,  // 36
        PLACE_HOLDER_37,  // 37
        PLACE_HOLDER_38,  // 38
        PLACE_HOLDER_39,  // 39
        PLACE_HOLDER_40,  // 40
        PLACE_HOLDER_41,  // 41
        PLACE_HOLDER_42,  // 42
        PLACE_HOLDER_43,  // 43
        PLACE_HOLDER_44,  // 44
        PLACE_HOLDER_45,  // 45
        PLACE_HOLDER_46,  // 46
        PLACE_HOLDER_47,  // 47
        PLACE_HOLDER_48,  // 48
        PLACE_HOLDER_49,  // 49
        PLACE_HOLDER_50,  // 50


        /***** WiFi to Host commands *****/
        // MAC addresses
        ASK_BLE_MAC,  // 51: Ask BLE MAC Address
        RECEIVE_WIFI_MAC,  // 52: Send WiFi MAC Address
        // Wifi Config
        WIFI_CONFIRM_NEW_CREDENTIALS,  // 53: Config msg reception confirmation
        WIFI_CONFIRM_NEW_STATIC_CONFIG,  // 54: Config msg reception confirmation
        // Wifi commands
        WIFI_REQUEST_ACTION,  // 55: Ask if WiFi should wake up
        WIFI_CONFIRM_ACTION,  // 56: Confirm reception of command
        // Wifi status alerting
        WIFI_ALERT_NO_SAVED_CREDENTIALS,  // 57: Alert WiFi has no saved credential
        WIFI_ALERT_CONNECTED,  // 58: Alert WiFi is connected
        WIFI_ALERT_DISCONNECTED,  // 59: Alert WiFi is disconnected
        WIFI_ALERT_SLEEPING,  // 60: Notify WiFi is sleeping
        WIFI_ALERT_AWAKE,  // 61: Notify WiFi is waking up
        // Data publication
        WIFI_CONFIRM_PUBLICATION,  // 62: Confirm publication of data
        // Cloud command reception and transmission
        CONFIG_FORWARD_CMD,  // 63: Forward a command string received by WiFi
        CONFIG_FORWARD_LEGACY_CMD,  // 64: Forward a command string received by WiFi (legacy)
        OTA_INITIATE_UPDATE,  // 65: Initiate OTA update
        OTA_RECEIVE_DATA,  // 66: Receive a packet of OTA data
    };
}

#endif // MSPCOMMANDS_H_INCLUDED
