#ifndef MSPCOMMANDS_H_INCLUDED
#define MSPCOMMANDS_H_INCLUDED


namespace MSPCommand
{
    enum command : uint8_t
    {
        // STM32 to WIFI commands
        RECEIVE_BLE_MAC,  // Send BLE MAC Address
        WIFI_WAKE_UP,  // Notify WiFi to wake up
        WIFI_DEEP_SLEEP,  // Deep-sleep WiFi  - optional duration (uint16_t in seconds)
        WIFI_HARD_RESET,  // WiFi Hard Reset
        WIFI_RECEIVE_SSID,  // Send WiFi SSID
        WIFI_RECEIVE_PASSWORD,  // Send WiFi Password
        WIFI_FORGET_CREDENTIALS,  // Forget saved WiFi credentials
        WIFI_CONNECT,  // Connect WiFi using saved credentials
        WIFI_DISCONNECT,  // Disconnect WiFi and turn off WiFi STA mode
        PUBLISH_RAW_DATA,  // Publish raw data
        PUBLISH_FEATURE,  // Publish features
        PUBLISH_DIAGNOSTIC,  // Publish diagnostic

        // WIFI to STM32 commands
        ASK_BLE_MAC,  // Ask BLE MAC Address
        WIFI_ALERT_NO_SAVED_CREDENTIALS,  // Alert WiFi has no saved credential
        WIFI_ALERT_CONNECTED,  // Alert WiFi is connected
        WIFI_ALERT_DISCONNECTED,  // Alert WiFi is disconnected
        WIFI_ALERT_AWAKE,  // Notify WiFi is awake
        WIFI_ALERT_SLEEPING,  // Notify WiFi is sleeping
        WIFI_REQUEST_ACTION,  // Ask if WiFi should wake up
        CONFIG_FORWARD_CMD,  // Forward a command string received by WiFi
        OTA_INITIATE_UPDATE,  // Initiate OTA update
        OTA_RECEIVE_DATA,  // receive a packet of OTA data

        // 255: NONE
        NONE = 255,
    };
}

#endif // MSPCOMMANDS_H_INCLUDED
