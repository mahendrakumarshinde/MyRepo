#ifndef BOARDDEFINITION_H
#define BOARDDEFINITION_H

#include <Arduino.h>


/* =============================================================================
    Device definition
============================================================================= */

/***** Firmware version *****/
const char FIRMWARE_VERSION[6] = "1.0.0";


/***** Device Type *****/
const uint8_t DEVICE_TYPE_LENGTH = 9;
const char DEVICE_TYPE[DEVICE_TYPE_LENGTH] = "ide_plus";


/* =============================================================================
    Debugging
============================================================================= */

/***** Test connection *****/
#if IUDEBUG_ANY == 1
    const char testSSID[20] = "The Port WiFi";  // "AndroidHotspot6994";  // "TP-LINK_3A67";
    const char testPSK[13] = "Th3P0rt317";  // "f1b94630f970";  // "tqN7*EnTsrP0";
    const IPAddress testStaticIP(0, 0, 0, 0);
    const IPAddress testGateway(0, 0, 0, 0);
    const IPAddress testSubnet(0, 0, 0, 0);
    const char hostMacAddress[18] = "94:54:93:10:63:DE";
//    const char testSSID[19] = "AndroidHotspot6994";
//    const char testPSK[13] = "f1b94630f970";
#endif  // IUDEBUG_ANY == 1


/* =============================================================================
    Default configuration values
============================================================================= */

/***** Post endpoins *****/
// General
const uint16_t MAX_HOST_LENGTH = 100;
const uint16_t MAX_ROUTE_LENGTH = 100;
const char DATA_DEFAULT_ENDPOINT_HOST[45] =
    "ideplus-dot-infinite-uptime-1232.appspot.com";
const uint16_t DATA_DEFAULT_ENDPOINT_PORT = 80;
// Raw data
const char RAW_DATA_DEFAULT_ENDPOINT_ROUTE[15] = "/raw_data?mac=";
// Feature data
const char FEATURE_DEFAULT_ENDPOINT_ROUTE[19] = "/mailbox/data?mac=";
// Diagnostic data
const char DIAGNOSTIC_DEFAULT_ENDPOINT_ROUTE[26] = "/mailbox/diagnostics?mac=";


/***** MQTT server and credentials *****/
// US-WEST1-A server
//IPAddress MQTT_DEFAULT_SERVER_IP(35, 197, 32, 136);
// ASIA-SOUTH1-A (Mumbai) server
const IPAddress MQTT_DEFAULT_SERVER_IP(35, 200, 183, 103);
const uint16_t MQTT_DEFAULT_SERVER_PORT = 1883;

const uint8_t MQTT_CREDENTIALS_MAX_LENGTH = 25;
const char MQTT_DEFAULT_USERNAME[9] = "ide_plus";
const char MQTT_DEFAULT_ASSWORD[13] = "nW$Pg81o@EJD";


// IU format compliance for message publication
const uint8_t CUSTOMER_PLACEHOLDER_LENGTH = 9;
const char CUSTOMER_PLACEHOLDER[CUSTOMER_PLACEHOLDER_LENGTH] = "XXXAdmin";


/* =============================================================================
    PubSub topic names
============================================================================= */

// Define TEST_TOPICS to use the PubSub test topics (instead of prod ones)
//#define TEST_TOPICS


#ifdef TEST_TOPICS
    const uint8_t FEATURE_TOPIC_LENGTH = 20;
    const uint8_t DIAGNOSTIC_TOPIC_LENGTH = 14;
    const uint8_t CHECKSUM_TOPIC_LENGTH = 20;
//    const uint8_t RAW_DATA_TOPIC_LENGTH = 17;
    const char FEATURE_TOPIC[FEATURE_TOPIC_LENGTH] = "iu_device_data_test";
    const char DIAGNOSTIC_TOPIC[DIAGNOSTIC_TOPIC_LENGTH] = "iu_error_test";
    const char CHECKSUM_TOPIC[CHECKSUM_TOPIC_LENGTH] = "config_confirm_test";
//    const char RAW_DATA_TOPIC[RAW_DATA_TOPIC_LENGTH] = "iu_raw_data_test";
#else
    const uint8_t FEATURE_TOPIC_LENGTH = 15;
    const uint8_t DIAGNOSTIC_TOPIC_LENGTH = 9;
    const uint8_t CHECKSUM_TOPIC_LENGTH = 15;
//    const uint8_t RAW_DATA_TOPIC_LENGTH = 12;
    const char FEATURE_TOPIC[FEATURE_TOPIC_LENGTH] = "iu_device_data";
    const char DIAGNOSTIC_TOPIC[DIAGNOSTIC_TOPIC_LENGTH] = "iu_error";
    const char CHECKSUM_TOPIC[CHECKSUM_TOPIC_LENGTH] = "config_confirm";
//    const char RAW_DATA_TOPIC[RAW_DATA_TOPIC_LENGTH] = "iu_raw_data";
#endif  // TEST_TOPICS


#endif  // BOARDDEFINITION_H
