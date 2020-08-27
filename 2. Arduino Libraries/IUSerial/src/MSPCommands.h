#ifndef MSPCOMMANDS_H_INCLUDED
#define MSPCOMMANDS_H_INCLUDED


namespace MSPCommand
{
    enum command : uint8_t
    {
        /***** Generic codes *****/
        NONE,  // 0: No command
        MSP_INVALID_CHECKSUM, // 1: Invalid checksum
        MSP_TOO_LONG, // 2: Received message is too long (doesn't fit in RX buffer)

        /***** Logging commands *****/
        SEND_LOG_MSG,

        /***** Host to WiFi commands *****/
        // MAC addresses
        RECEIVE_BLE_MAC,  // Send BLE MAC Address
        ASK_WIFI_MAC,  // Ask WiFi MAC Address
        ASK_WIFI_FV,    // Ask Wifi firmware version
	/* ****** Firmware Version and FFT configuration commands: Host to Wifi ******/
	RECEIVE_HOST_FIRMWARE_VERSION,
        RECEIVE_HOST_SAMPLING_RATE,
        RECEIVE_HOST_BLOCK_SIZE,
        PUBLISH_DEVICE_DETAILS_MQTT,
	/* ****** Firmware Version Commands Host to Wifi ******/
	// Wifi Config
        ASK_WIFI_CONFIG,
        SEND_WIFI_CONFIG,
        WIFI_RECEIVE_AUTH_TYPE,
        WIFI_RECEIVE_SSID,  // Send WiFi SSID
        WIFI_RECEIVE_PASSWORD,  // Send WiFi Password
        WIFI_FORGET_CREDENTIALS,  // Forget saved WiFi credentials
        WIFI_RECEIVE_STATIC_IP,  // Send Static IP
        WIFI_RECEIVE_GATEWAY,  // Send Gateway (for Static IP)
        WIFI_RECEIVE_SUBNET,  // Send Subnet (for Static IP)
        WIFI_FORGET_STATIC_CONFIG,  // Forget static IP, gateway & subnet
        WIFI_RECEIVE_DNS1, 
        WIFI_RECEIVE_DNS2,
        
        
        // Wifi commands
        WIFI_SOFT_RESET,  // WiFi Soft Reset
        WIFI_CONNECT,  // Connect WiFi using saved credentials
        WIFI_DISCONNECT,  // Disconnect WiFi and turn off WiFi STA mode
        // Data publication
	SEND_RAW_DATA,	   //send raw data command	
        PUBLISH_RAW_DATA,  // Publish raw data
        PUBLISH_FEATURE,  // Publish features
        PUBLISH_FEATURE_WITH_CONFIRMATION,  // Publish features and confirm
        PUBLISH_DIAGNOSTIC,  // Publish diagnostic
        PUBLISH_CONFIG_CHECKSUM,  // Publish configuration checksum
        // Cloud command reception and transmission
        HOST_CONFIRM_RECEPTION,  // Confirm command reception
        HOST_FORWARD_CMD,  // Forward a command string received by the host
        // Publication protocol
        PUBLICATION_USE_MQTT,
        PUBLICATION_USE_HTTP,
        // Settable parameters (addresses, credentials, etc)
        SET_RAW_DATA_ENDPOINT_HOST,
        SET_RAW_DATA_ENDPOINT_ROUTE,
        SET_RAW_DATA_ENDPOINT_PORT,
	//SET_HTTP_CLIENT_FLAG,	
        SET_MQTT_SERVER_IP,
        SET_MQTT_SERVER_PORT,
        SET_MQTT_USERNAME,
        SET_MQTT_PASSWORD,
	// Diagnostic Fingerprint commands (Host - Wifi )
	RECEIVE_DIAGNOSTIC_ACK,
	SEND_DIAGNOSTIC_RESULTS,
	// Http Acknowledge commands
	RECEIVE_HTTP_CONFIG_ACK,
	GET_PENDING_HTTP_CONFIG,
	SET_PENDING_HTTP_CONFIG,
	SEND_ACCOUNTID,
        WIFI_RECEIVE_USERPSWD,
        WIFI_RECEIVE_USERNAME,

        /***** WiFi to Host commands *****/
        HTTP_ACK,                       // Used to send back HTTP status code to the MCU
        // MAC addresses
        ASK_BLE_MAC,  // Ask BLE MAC Address
        RECEIVE_WIFI_MAC,  // Send WiFi MAC Address
        RECEIVE_WIFI_FV,   // Host will receive WiFi Firmware Version
	
	/******* wifi to Host Commands ******/
	ASK_HOST_FIRMWARE_VERSION,
        ASK_HOST_SAMPLING_RATE,                         // in Hz
        ASK_HOST_BLOCK_SIZE,

        // Wifi Config
        WIFI_CONFIRM_NEW_CREDENTIALS,  // Config msg reception confirmation
        WIFI_CONFIRM_NEW_STATIC_CONFIG,  // Config msg reception confirmation
        // Wifi commands
        WIFI_CONFIRM_ACTION,  // Confirm reception of command
        // Wifi status alerting
        WIFI_ALERT_NO_SAVED_CREDENTIALS,  // Alert WiFi has no saved credential
        WIFI_ALERT_CONNECTED,  // Alert WiFi is connected
        WIFI_ALERT_DISCONNECTED,  // Alert WiFi is disconnected
        WIFI_ALERT_SLEEPING,  // Notify WiFi is sleeping
        WIFI_ALERT_AWAKE,  // Notify WiFi is waking up
        // Data publication
        WIFI_CONFIRM_PUBLICATION,  // Confirm publication of data
	RECEIVE_RAW_DATA_ACK,	
        // Cloud command reception and transmission
        SET_DATETIME,
        CONFIG_FORWARD_CONFIG,  // Forward a config string received by WiFi
        CONFIG_FORWARD_CMD,  // Forward a command string received by WiFi
        CONFIG_FORWARD_LEGACY_CMD,  // Forward a command string received by WiFi (legacy)
        OTA_INITIATE_UPDATE,  // Initiate OTA update
        OTA_RECEIVE_DATA,  // Receive a packet of OTA data
        // Settable parameters (addresses, credentials, etc)
        GET_RAW_DATA_ENDPOINT_INFO,
        GET_MQTT_CONNECTION_INFO,
	
	// Diagnostic Fingerprint commads (Wifi - Host )
	SEND_DIAGNOSTIC_ACK,
	RECEIVE_DIAGNOSTIC_RESULTS,
	GET_ACCOUNTID,
        ESP_DEBUG_TO_STM_HOST,
        ESP32_RESET,
        SET_OTA_STM_URI,
        SET_OTA_ESP_URI,
        OTA_INIT_ACK,
        OTA_FDW_START,
        OTA_FDW_SUCCESS,
        OTA_FDW_ABORT,
        OTA_DNLD_FAIL,
        OTA_FUG_START,
        OTA_FUG_SUCCESS,
        OTA_FUG_ABORT,
        OTA_PACKET_DATA,
        OTA_PACKET_ACK,
        OTA_PACKET_NAK,
        OTA_STM_DNLD_OK,
        OTA_ESP_DNLD_OK,
        OTA_STM_DNLD_STATUS,
        OTA_ESP_DNLD_STATUS,
        OTA_CONFIRM_FUG_START,
        OTA_CONFIRM_FDW_ABORT,
        OTA_CONFIRM_FDW_SUCCESS,
        OTA_CONFIRM_FUG_ABORT,
        OTA_CONFIRM_FUG_SUCCESS,
        SEND_FLASH_STATUS,
        SEND_SENSOR_STATUS,
        GET_DEVICE_CONFIG,
        WIFI_GET_TX_POWER,
        WIFI_SET_TX_POWER,
        OTA_INIT_REQUEST,
	GET_ESP_RSSI,
        SET_TLS_CERT_URI,
        SET_TLS_KEY_URI,
        TLS_INIT_ACK,
        SET_DIAGNOSTIC_URL,
        CERT_DOWNLOAD_INIT_ACK,
        GET_CERT_DOWNLOAD_CONFIG,
        CERT_UPGRADE_INIT,
        CERT_UPGRADE_SUCCESS,
        CERT_UPGRADE_ABORT,
        CERT_DOWNLOAD_ABORT,
        CERT_DOWNLOAD_SUCCESS,
        CERT_NO_DOWNLOAD_AVAILABLE,
        ALL_MQTT_CONNECT_ATTEMPT_FAILED,
        CERT_UPGRADE_TRIGGER,
        CERT_DOWNLOAD_INIT,
        MQTT_CONNECT,
        MQTT_DISCONNECT,
        SET_MQTT_TLS_FLAG,
        UPGRADE_TLS_SSL_START,
        DOWNLOAD_TLS_SSL_START,
        SET_CERT_CONFIG_URL,
        CERT_INVALID_STATIC_URL,
        ESP32_FLASH_FILE_WRITE_FAILED,
        ESP32_FLASH_FILE_READ_FAILED,
        DELETE_CERT_FILES,
        READ_CERTS,
        MQTT_ALERT_CONNECTED,
        MQTT_ALERT_DISCONNECTED,
        GET_CERT_COMMOM_URL,
        SET_CERT_DOWNLOAD_MSGID,
        SEND_WIFI_HASH,
        GET_CERT_CONFIG,
        SEND_CERT_DWL_CFG,
        SEND_CERT_DIG_CFG,
        GOOGLE_TIME_QUERY,
        PUBLISH_FIRMWARE_VER,
        // Config update (Host - Wifi)
        CONFIG_ACK
	// ESP32 MSP Commands
	//ESP_DEBUG_TO_STM_HOST
    };
}

#endif // MSPCOMMANDS_H_INCLUDED
