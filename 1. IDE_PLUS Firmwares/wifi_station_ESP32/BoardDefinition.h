#ifndef BOARDDEFINITION_H
#define BOARDDEFINITION_H

#include <Arduino.h>


/* =============================================================================
    Device definition
============================================================================= */

/***** Firmware version *****/
const char FIRMWARE_VERSION[6] = "3.0.5";


/***** Device Type *****/
const uint8_t DEVICE_TYPE_LENGTH = 9;
const char DEVICE_TYPE[DEVICE_TYPE_LENGTH] = "ide_plus";


/* =============================================================================
    Debugging
============================================================================= */

/***** Test connection *****/
#if IUDEBUG_ANY == 1
    const char testSSID[20] = "The Port WiFi";  // "AndroidHotspot6994";
    const char testPSK[13] = "Th3P0rt317";  // "f1b94630f970";
    const IPAddress testStaticIP(0, 0, 0, 0);
    const IPAddress testGateway(0, 0, 0, 0);
    const IPAddress testSubnet(0, 0, 0, 0);
    const char hostMacAddress[18] = "94:54:93:10:7C:0B";
#endif  // IUDEBUG_ANY == 1


/* =============================================================================
    Default configuration values
============================================================================= */
// http://115.112.92.146:58888/contineonx-web-admin/imiot-infiniteuptime-api/postdatadump
//ideplus-dot-infinite-uptime-1232.appspot.com:80/raw_data?mac=;
/***** Post endpoins *****/
// General
const uint16_t MAX_HOST_LENGTH = 100;
const uint16_t MAX_ROUTE_LENGTH = 100;
const char DATA_DEFAULT_ENDPOINT_HOST[45] =    "api-idap.infinite-uptime.com";              //"ideplus-dot-infinite-uptime-1232.appspot.com";                                                           //                              
const uint16_t DATA_DEFAULT_ENDPOINT_PORT = 80;                                   //80;                                                              
// Raw data
const char RAW_DATA_DEFAULT_ENDPOINT_ROUTE[70] =  "/api/2.0/datalink/http_dump_v2";                  //"/raw_data?mac=";                                            
// Feature data
const char FEATURE_DEFAULT_ENDPOINT_ROUTE[19] = "/mailbox/data?mac=";
// Diagnostic data
const char DIAGNOSTIC_DEFAULT_ENDPOINT_ROUTE[26] = "/mailbox/diagnostics?mac=";


// IU format compliance for message publication
const uint8_t CUSTOMER_PLACEHOLDER_LENGTH = 9;
const char CUSTOMER_PLACEHOLDER[CUSTOMER_PLACEHOLDER_LENGTH] = "XXXAdmin";

/* =============================================================================
    Default MQTT Endpoints
============================================================================= */
const uint8_t MQTT_CREDENTIALS_MAX_LENGTH = 50;
const char MQTT_DEFAULT_SERVER_IP[MQTT_CREDENTIALS_MAX_LENGTH] = "mqtt.infinite-uptime.com";
const uint16_t MQTT_DEFAULT_SERVER_PORT = 1883;             
const char MQTT_DEFAULT_USERNAME[MQTT_CREDENTIALS_MAX_LENGTH] =  "iuprod";         
const char MQTT_DEFAULT_PASSWORD[MQTT_CREDENTIALS_MAX_LENGTH] =  "iuprod";         
const bool MQTT_DEFAULT_TLS_FLAG = false;
/* =============================================================================
    Diagnostic HTTP/HTTPS Endpoints
============================================================================= */

const char DIAGNOSTIC_DEFAULT_ENDPOINT_HOST[MAX_HOST_LENGTH] = "http://15.206.97.181";
const char DIAGNOSTIC_DEFAULT_ENDPOINT_PATH[MAX_ROUTE_LENGTH] = "/certificates/download-status";
const uint16_t DIAGNOSTIC_DEFAULT_ENDPOINT_PORT = 8000;

/* =============================================================================
    certificates Config  HTTP/HTTPS Endpoints
============================================================================= */

const char CERT_CONFIG_DEFAULT_ENDPOINT_HOST[MAX_HOST_LENGTH] = "http://15.206.97.181";
const char CERT_CONFIG_DEFAULT_ENDPOINT_PATH[MAX_ROUTE_LENGTH] = "/certificates?deviceIdentifier=";
const uint16_t CERT_CONFIG_DEFAULT_ENDPOINT_PORT = 8000;

/* =============================================================================
    certificates Download/Upgrade Default MessageID
============================================================================= */
const char CERT_DOWNLOAD_DEFAULT_MESSAGEID[100] = "0123456789_cert_default_messageid";

/* =============================================================================
    PubSub topic names
============================================================================= */

// Define TEST_TOPICS to use the PubSub test topics (instead of prod ones)
//#define TEST_TOPICS


#ifdef TEST_TOPICS
    const uint8_t FEATURE_TOPIC_LENGTH = 20;
    const uint8_t DIAGNOSTIC_TOPIC_LENGTH = 14;
    const uint8_t CHECKSUM_TOPIC_LENGTH = 20;
    const uint8_t LOG_TOPIC_LENGTH = 14;
    const uint8_t FINGERPRINT_TOPIC_LENGTH = 25;
    
//    const uint8_t RAW_DATA_TOPIC_LENGTH = 17;
    const char FEATURE_TOPIC[FEATURE_TOPIC_LENGTH] = "iu_device_data_test";
    const char DIAGNOSTIC_TOPIC[DIAGNOSTIC_TOPIC_LENGTH] = "iu_error_test";
    const char CHECKSUM_TOPIC[CHECKSUM_TOPIC_LENGTH] = "config_confirm_test";
    const char LOG_TOPIC[LOG_TOPIC_LENGTH] = "ide_logs_test";
//    const char RAW_DATA_TOPIC[RAW_DATA_TOPIC_LENGTH] = "iu_raw_data_test";
#else
    const uint8_t CERT_TOPIC_LENGTH = 21;
    const uint8_t OTA_TOPIC_LENGTH = 20;
    const uint8_t OTA_P_TOPIC_LENGTH = 22;
    const uint8_t FEATURE_TOPIC_LENGTH = 15;
    const uint8_t DIAGNOSTIC_TOPIC_LENGTH = 9;
    const uint8_t CHECKSUM_TOPIC_LENGTH = 15;
    const uint8_t LOG_TOPIC_LENGTH = 9;
    //Diagnostic  Fingerprint topic lengths
    const uint8_t FINGERPRINT_TOPIC_LENGTH = 31;
    const uint8_t FINGERPRINT_ACK_TOPIC_LENGTH = 27;  // not in use
    const uint8_t COMMAND_RESPONSE_TOPIC_LENGTH = 27;
    const uint8_t IU_DIG_RULE_ENGINE_TOPIC_LENGTH = 50;  
    const uint8_t IU_FRES_TOPIC_LENGTH = 50;  
//   const uint8_t RAW_DATA_TOPIC_LENGTH = 12;
    const char CERT_STATUS_TOPIC[CERT_TOPIC_LENGTH] = "ide_plus/cert/status";
    const char OTA_TOPIC[OTA_TOPIC_LENGTH] = "ide_plus/ota/status";
    const char OTA_P_TOPIC[OTA_P_TOPIC_LENGTH] = "ide_plus/ota/progress";
    const char FEATURE_TOPIC[FEATURE_TOPIC_LENGTH] = "iu_device_data";
    const char DIAGNOSTIC_TOPIC[DIAGNOSTIC_TOPIC_LENGTH] = "iu_error";
    const char CHECKSUM_TOPIC[CHECKSUM_TOPIC_LENGTH] = "config_confirm";
    const char LOG_TOPIC[LOG_TOPIC_LENGTH] = "ide_logs";
//    const char RAW_DATA_TOPIC[RAW_DATA_TOPIC_LENGTH] = "iu_raw_data";
    const char FINGERPRINT_DATA_PUBLISH_TOPIC [FINGERPRINT_TOPIC_LENGTH] = "ide_plus/iu_fingerprints/data"; // publish data here
// const char FINGERPRINT_ACK[FINGERPRINT_ACK_TOPIC_LENGTH] = "ide_plus/command_response/";                 // not in use
    const char COMMAND_RESPONSE_TOPIC[COMMAND_RESPONSE_TOPIC_LENGTH] = "ide_plus/command_response/";      //// publish ack here
    const char IU_DIG_RULE_ENGINE_TOPIC[IU_DIG_RULE_ENGINE_TOPIC_LENGTH] = "iu_device_data/ts/diagnostic";
    const char IU_RDIG_RULE_ENGINE_TOPIC[IU_DIG_RULE_ENGINE_TOPIC_LENGTH] = "iu_device_data/alarm/diagnostic";       //// publish IU Rule Engine Data
    const char IU_FRES[IU_FRES_TOPIC_LENGTH] = "iu_device_data/ts/advanced_feature";      //// publish IU Rule Engine Data
    

#endif  // TEST_TOPICS


#endif  // BOARDDEFINITION_H
