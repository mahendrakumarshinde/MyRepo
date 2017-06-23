#ifndef config_h
#define config_h

/*************************************************************************************************/
/*************                                                                     ***************/
/*************                     SECTION  1 - BASIC SETUP                        ***************/
/*************                                                                     ***************/
/*************************************************************************************************/

    // Uncomment one sensor definition only
    #define SENSOR_0
    //#define SENSOR_1
    //#define SENSOR_2
    //#define SENSOR_3
    //#define SENSOR_4
    
    //#define SERIAL_DEBUG
    #define BRIDGE_STAT_UPDATE_TARGET     1000000                                           // 1Hz
    #define UPDATE_SIZE                   20
    #define WIFI_LED_PIN                  15

    // Sensor UDP port and IP address definitions
    #ifdef SENSOR_0
      #define TX_PORT                     9048
      #define IP_CLIENT                   ipClient(192,168,4,10)
      #define OTA_RX_PORT                 9053
      #define OTA_TX_PORT                 9054
    #endif
    #ifdef SENSOR_1
      #define TX_PORT                     9049
      #define IP_CLIENT                   ipClient(192,168,4,11)
      #define OTA_RX_PORT                 9053
      #define OTA_TX_PORT                 9054
    #endif
    #ifdef SENSOR_2
      #define TX_PORT                     9050
      #define IP_CLIENT                   ipClient(192,168,4,12)
      #define OTA_RX_PORT                 9053
      #define OTA_TX_PORT                 9054
    #endif
    #ifdef SENSOR_3
      #define TX_PORT                     9051
      #define IP_CLIENT                   ipClient(192,168,4,13)
      #define OTA_RX_PORT                 9053
      #define OTA_TX_PORT                 9054
    #endif
    #ifdef SENSOR_4
      #define TX_PORT                     9052
      #define IP_CLIENT                   ipClient(192,168,4,14)
      #define OTA_RX_PORT                 9053
      #define OTA_TX_PORT                 9054
    #endif
/*************************************************************************************************/

#endif
